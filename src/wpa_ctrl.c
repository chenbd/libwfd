/*
 * libwfd - Wifi-Display/Miracast Protocol Implementation
 *
 * Copyright (c) 2013-2014 David Herrmann <dh.herrmann@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "libwfd.h"
#include "shl_macro.h"

#define CTRL_PATH_TEMPLATE "/tmp/libwfd-wpa-ctrl-%d-%lu"
#define REQ_REPLY_MAX 512

#ifndef UNIX_PATH_MAX
#  define UNIX_PATH_MAX (sizeof(((struct sockaddr_un*)0)->sun_path))
#endif

struct wfd_wpa_ctrl {
	unsigned long ref;
	wfd_wpa_ctrl_event_t event_fn;
	void *data;
	sigset_t mask;
	int efd;
	int tfd;

	int req_fd;
	char req_name[UNIX_PATH_MAX];
	int ev_fd;
	char ev_name[UNIX_PATH_MAX];
};

static int wpa_request(int fd, const void *cmd, size_t cmd_len,
		       void *reply, size_t *reply_len, int64_t *t2,
		       const sigset_t *mask);
static int wpa_request_ok(int fd, const void *cmd, size_t cmd_len, int64_t *t,
			  const sigset_t *mask);

static int64_t get_time_us(void)
{
	int64_t t;
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	t = ts.tv_sec * 1000LL * 1000LL;
	t += ts.tv_nsec / 1000LL;

	return t;
}

static void us_to_timespec(struct timespec *ts, int64_t us)
{
	ts->tv_sec = us / (1000LL * 1000LL);
	ts->tv_nsec = (us % (1000LL * 1000LL)) * 1000LL;
}

_shl_public_
int wfd_wpa_ctrl_new(wfd_wpa_ctrl_event_t event_fn, void *data,
		     struct wfd_wpa_ctrl **out)
{
	struct wfd_wpa_ctrl *wpa;
	struct epoll_event ev;
	int r;

	if (!out || !event_fn)
		return -EINVAL;

	wpa = calloc(1, sizeof(*wpa));
	if (!wpa)
		return -ENOMEM;
	wpa->ref = 1;
	wpa->event_fn = event_fn;
	wpa->data = data;
	wpa->efd = -1;
	wpa->tfd = -1;
	wpa->req_fd = -1;
	wpa->ev_fd = -1;
	sigemptyset(&wpa->mask);

	wpa->efd = epoll_create1(EPOLL_CLOEXEC);
	if (wpa->efd < 0) {
		r = -errno;
		goto err_wpa;
	}

	wpa->tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
	if (wpa->tfd < 0) {
		r = -errno;
		goto err_efd;
	}

	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLHUP | EPOLLERR | EPOLLIN;
	ev.data.ptr = &wpa->tfd;

	r = epoll_ctl(wpa->efd, EPOLL_CTL_ADD, wpa->tfd, &ev);
	if (r < 0) {
		r = -errno;
		goto err_tfd;
	}

	*out = wpa;
	return 0;

err_tfd:
	close(wpa->tfd);
err_efd:
	close(wpa->efd);
err_wpa:
	free(wpa);
	return r;
}

_shl_public_
void wfd_wpa_ctrl_ref(struct wfd_wpa_ctrl *wpa)
{
	if (!wpa || !wpa->ref)
		return;

	++wpa->ref;
}

_shl_public_
void wfd_wpa_ctrl_unref(struct wfd_wpa_ctrl *wpa)
{
	if (!wpa || !wpa->ref || --wpa->ref)
		return;

	wfd_wpa_ctrl_close(wpa);
	close(wpa->tfd);
	close(wpa->efd);
	free(wpa);
}

_shl_public_
void wfd_wpa_ctrl_set_data(struct wfd_wpa_ctrl *wpa, void *data)
{
	if (!wpa)
		return;

	wpa->data = data;
}

_shl_public_
void *wfd_wpa_ctrl_get_data(struct wfd_wpa_ctrl *wpa)
{
	if (!wpa)
		return NULL;

	return wpa->data;
}

static int bind_socket(int fd, char *name)
{
	static unsigned long real_counter;
	unsigned long counter;
	struct sockaddr_un src;
	int r;
	bool tried = false;

	/* TODO: make wpa_supplicant allow unbound clients */

	/* Yes, this counter is racy, but wpa_supplicant doesn't provide support
	 * for unbound clients (it crashes..). We could add a current-time based
	 * random part, but that might leave stupid pipes around in /tmp. So
	 * lets just use this internal counter and blame
	 * wpa_supplicant.. Yey! */
	counter = real_counter++;

	memset(&src, 0, sizeof(src));
	src.sun_family = AF_UNIX;

	/* Ugh! mktemp() is racy but stupid wpa_supplicant requires us to bind
	 * to a real file. Older versions will segfault if we don't... */
	snprintf(name, UNIX_PATH_MAX - 1, CTRL_PATH_TEMPLATE,
		 (int)getpid(), counter);
	name[UNIX_PATH_MAX - 1] = 0;

try_again:
	strcpy(src.sun_path, name);

	r = bind(fd, (struct sockaddr*)&src, sizeof(src));
	if (r < 0) {
		if (errno == EADDRINUSE && !tried) {
			tried = true;
			unlink(name);
			goto try_again;
		}

		return -errno;
	}

	return 0;
}

static int connect_socket(int fd, const char *ctrl_path)
{
	int r;
	struct sockaddr_un dst;
	size_t len;

	memset(&dst, 0, sizeof(dst));
	dst.sun_family = AF_UNIX;

	len = strlen(ctrl_path);
	if (!strncmp(ctrl_path, "@abstract:", 10)) {
		if (len > sizeof(dst.sun_path) - 2)
			return -EINVAL;

		dst.sun_path[0] = 0;
		dst.sun_path[sizeof(dst.sun_path) - 1] = 0;
		strncpy(&dst.sun_path[1], &ctrl_path[10],
			sizeof(dst.sun_path) - 2);
	} else {
		if (len > sizeof(dst.sun_path) - 1)
			return -EINVAL;

		dst.sun_path[sizeof(dst.sun_path) - 1] = 0;
		strncpy(dst.sun_path, ctrl_path, sizeof(dst.sun_path) - 1);
	}

	r = connect(fd, (struct sockaddr*)&dst, sizeof(dst));
	if (r < 0)
		return -errno;

	return 0;
}

static int open_socket(struct wfd_wpa_ctrl *wpa, const char *ctrl_path,
		       void *data, char *name)
{
	int fd, r;
	struct epoll_event ev;

	fd = socket(PF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (fd < 0)
		return -errno;

	r = bind_socket(fd, name);
	if (r < 0)
		goto err_fd;

	r = connect_socket(fd, ctrl_path);
	if (r < 0)
		goto err_name;

	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLHUP | EPOLLERR | EPOLLIN;
	ev.data.ptr = data;
	r = epoll_ctl(wpa->efd, EPOLL_CTL_ADD, fd, &ev);
	if (r < 0) {
		r = -errno;
		goto err_name;
	}

	return fd;

err_name:
	unlink(name);
err_fd:
	close(fd);
	return r;
}

static void close_socket(struct wfd_wpa_ctrl *wpa, int fd, char *name)
{
	epoll_ctl(wpa->efd, EPOLL_CTL_DEL, fd, NULL);
	unlink(name);
	close(fd);
}

static int arm_timer(struct wfd_wpa_ctrl *wpa, int64_t usecs)
{
	struct itimerspec spec;
	int r;

	us_to_timespec(&spec.it_value, usecs);
	spec.it_interval = spec.it_value;

	r = timerfd_settime(wpa->tfd, 0, &spec, NULL);
	if (r < 0)
		return -errno;

	return 0;
}

static void disarm_timer(struct wfd_wpa_ctrl *wpa)
{
	arm_timer(wpa, 0);
}

_shl_public_
int wfd_wpa_ctrl_open(struct wfd_wpa_ctrl *wpa, const char *ctrl_path)
{
	int r;

	if (!wpa || !ctrl_path)
		return -EINVAL;
	if (wfd_wpa_ctrl_is_open(wpa))
		return -EALREADY;

	/* 10s PING timer for timeouts */
	r = arm_timer(wpa, 10000000LL);
	if (r < 0)
		return r;

	wpa->req_fd = open_socket(wpa, ctrl_path, &wpa->req_fd, wpa->req_name);
	if (wpa->req_fd < 0) {
		r = wpa->req_fd;
		goto err_timer;
	}

	wpa->ev_fd = open_socket(wpa, ctrl_path, &wpa->ev_fd, wpa->ev_name);
	if (wpa->ev_fd < 0) {
		r = wpa->ev_fd;
		goto err_req;
	}

	r = wpa_request_ok(wpa->ev_fd, "ATTACH", 6, NULL, &wpa->mask);
	if (r < 0)
		goto err_ev;

	return 0;

err_ev:
	wpa_request_ok(wpa->ev_fd, "DETACH", 6, NULL, &wpa->mask);
	close_socket(wpa, wpa->ev_fd, wpa->ev_name);
	wpa->ev_fd = -1;
err_req:
	close_socket(wpa, wpa->req_fd, wpa->req_name);
	wpa->req_fd = -1;
err_timer:
	disarm_timer(wpa);
	return r;
}

_shl_public_
void wfd_wpa_ctrl_close(struct wfd_wpa_ctrl *wpa)
{
	if (!wpa || !wfd_wpa_ctrl_is_open(wpa))
		return;

	wpa_request_ok(wpa->ev_fd, "DETACH", 6, NULL, &wpa->mask);

	close_socket(wpa, wpa->ev_fd, wpa->ev_name);
	wpa->ev_fd = -1;

	close_socket(wpa, wpa->req_fd, wpa->req_name);
	wpa->req_fd = -1;

	disarm_timer(wpa);
}

_shl_public_
bool wfd_wpa_ctrl_is_open(struct wfd_wpa_ctrl *wpa)
{
	return wpa && wpa->ev_fd >= 0;
}

_shl_public_
int wfd_wpa_ctrl_get_fd(struct wfd_wpa_ctrl *wpa)
{
	return wpa ? wpa->efd : -1;
}

_shl_public_
void wfd_wpa_ctrl_set_sigmask(struct wfd_wpa_ctrl *wpa, const sigset_t *mask)
{
	if (!wpa || !mask)
		return;

	memcpy(&wpa->mask, mask, sizeof(sigset_t));
}

static int read_ev(struct wfd_wpa_ctrl *wpa)
{
	char buf[REQ_REPLY_MAX + 1];
	ssize_t l;

	do {
		l = recv(wpa->ev_fd, buf, sizeof(buf) - 1, MSG_DONTWAIT);
		if (l < 0) {
			if (errno == EAGAIN || errno == EINTR)
				return 0;
			else
				return -errno;
		} else if (l > 0) {
			if (l > sizeof(buf) - 1)
				l = sizeof(buf) - 1;
			buf[l] = 0;

			/* only handle event-msgs ('<') on ev-socket */
			if (*buf == '<')
				wpa->event_fn(wpa, wpa->data, buf, l);

			/* exit if the callback closed the connection */
			if (!wfd_wpa_ctrl_is_open(wpa))
				return -ENODEV;
		}
	} while (l > 0);

	return 0;
}

static int dispatch_ev(struct wfd_wpa_ctrl *wpa, const struct epoll_event *e)
{
	int r;

	if (e->events & EPOLLIN) {
		r = read_ev(wpa);
		if (r < 0)
			return r;
	}

	/* handle HUP/ERR last so we drain input first */
	if (e->events & (EPOLLHUP | EPOLLERR))
		return -EPIPE;

	return 0;
}

static int read_req(struct wfd_wpa_ctrl *wpa)
{
	char buf[REQ_REPLY_MAX];
	ssize_t l;

	/*
	 * Drain input queue on req-socket; we're not interested in spurious
	 * events on this fd so ignore any data.
	 */

	do {
		l = recv(wpa->req_fd, buf, sizeof(buf), MSG_DONTWAIT);
		if (l < 0) {
			if (errno == EAGAIN || errno == EINTR)
				return 0;
			else
				return -errno;
		}
	} while (l > 0);

	return 0;
}

static int dispatch_req(struct wfd_wpa_ctrl *wpa, const struct epoll_event *e)
{
	int r;

	if (e->events & EPOLLIN) {
		r = read_req(wpa);
		if (r < 0)
			return r;
	}

	/* handle HUP/ERR last so we drain input first */
	if (e->events & (EPOLLHUP | EPOLLERR))
		return -EPIPE;

	return 0;
}

static int read_tfd(struct wfd_wpa_ctrl *wpa)
{
	ssize_t l;
	uint64_t exp;
	int r;
	char buf[10];
	size_t len = sizeof(buf);

	/* Send PING request if the timer expires. If the wpa_supplicant
	 * doesn't respond in a timely manner, return an error. */

	l = read(wpa->tfd, &exp, sizeof(exp));
	if (l < 0 && errno != EAGAIN && errno != EINTR) {
		return -errno;
	} else if (l == sizeof(exp)) {
		r = wpa_request(wpa->req_fd, "PING", 4, buf, &len, NULL,
				&wpa->mask);
		if (r < 0)
			return r;
		if (len != 5 || strncmp(buf, "PONG\n", 5))
			return -ETIMEDOUT;
	}

	return 0;
}

static int dispatch_tfd(struct wfd_wpa_ctrl *wpa, const struct epoll_event *e)
{
	int r = 0;

	/* Remove tfd from epoll-set on HUP/ERR. This shouldn't happen, but if
	 * it does, just stop receiving events from it. */
	if (e->events & (EPOLLHUP | EPOLLERR)) {
		r = -EFAULT;
		goto error;
	}

	if (e->events & (EPOLLIN)) {
		r = read_tfd(wpa);
		if (r < 0)
			goto error;
	}

	return 0;

error:
	epoll_ctl(wpa->efd, EPOLL_CTL_DEL, wpa->tfd, NULL);
	return r;
}

_shl_public_
int wfd_wpa_ctrl_dispatch(struct wfd_wpa_ctrl *wpa, int timeout)
{
	struct epoll_event ev[2], *e;
	int r, n, i;
	const size_t max = sizeof(ev) / sizeof(*ev);

	if (!wfd_wpa_ctrl_is_open(wpa))
		return -ENODEV;

	n = epoll_wait(wpa->efd, ev, max, timeout);
	if (n < 0) {
		if (errno == EAGAIN || errno == EINTR)
			return 0;
		else
			return -errno;
	} else if (n > max) {
		n = max;
	}

	r = 0;
	for (i = 0; i < n; ++i) {
		e = &ev[i];
		if (e->data.ptr == &wpa->ev_fd)
			r = dispatch_ev(wpa, e);
		else if (e->data.ptr == &wpa->req_fd)
			r = dispatch_req(wpa, e);
		else if (e->data.ptr == &wpa->tfd)
			r = dispatch_tfd(wpa, e);

		if (r < 0)
			break;
	}

	return r;
}

static int timed_send(int fd, const void *cmd, size_t cmd_len,
		      int64_t *timeout, const sigset_t *mask)
{
	bool done = false;
	int64_t start, t;
	ssize_t l;
	int n;
	struct pollfd fds[1];
	const size_t max = sizeof(fds) / sizeof(*fds);
	struct timespec ts;

	start = get_time_us();

	do {
		memset(fds, 0, sizeof(fds));
		fds[0].fd = fd;
		fds[0].events = POLLHUP | POLLERR | POLLOUT;

		us_to_timespec(&ts, *timeout);
		n = ppoll(fds, max, &ts, mask);
		if (n < 0 && errno != EAGAIN) {
			return -errno;
		} else if (!n) {
			return -ETIMEDOUT;
		} else if (n > 0) {
			if (fds[0].revents & (POLLHUP | POLLERR))
				return -EPIPE;

			l = send(fd, cmd, cmd_len, MSG_NOSIGNAL);
			if (l < 0 && errno != EAGAIN && errno != EINTR) {
				return -errno;
			} else if (l > 0) {
				/* We don't care how much was sent. If we
				 * couldn't send the whole datagram, we still
				 * try to recv the error reply from
				 * wpa_supplicant. There's no way to split
				 * datagrams so we're screwed in that case. */
				done = true;
			}
		}

		/* recalculate remaining timeout */
		t = *timeout - (get_time_us() - start);
		if (t <= 0) {
			*timeout = 0;
			if (!done)
				return -ETIMEDOUT;
		} else {
			*timeout = t;
		}
	} while (!done);

	return 0;
}

static int timed_recv(int fd, void *reply, size_t *reply_len, int64_t *timeout,
		      const sigset_t *mask)
{
	bool done = false;
	int64_t start, t;
	ssize_t l;
	int n;
	struct pollfd fds[1];
	const size_t max = sizeof(fds) / sizeof(*fds);
	struct timespec ts;

	start = get_time_us();

	do {
		memset(fds, 0, sizeof(fds));
		fds[0].fd = fd;
		fds[0].events = POLLHUP | POLLERR | POLLIN;

		us_to_timespec(&ts, *timeout);
		n = ppoll(fds, max, &ts, mask);
		if (n < 0 && errno != EAGAIN) {
			return -errno;
		} else if (!n) {
			return -ETIMEDOUT;
		} else if (n > 0) {
			if (fds[0].revents & (POLLHUP | POLLERR))
				return -EPIPE;

			l = recv(fd, reply, *reply_len, MSG_DONTWAIT);
			if (l < 0 && errno != EAGAIN && errno != EINTR) {
				return -errno;
			} else if (l > 0 && *(char*)reply != '<') {
				/* We ignore any event messages ('<') on this
				 * fd as they're handled via a separate pipe.
				 * Return the received reply unchanged. */
				*reply_len = (l > *reply_len) ? *reply_len : l;
				done = true;
			}
		}

		/* recalculate remaining timeout */
		t = *timeout - (get_time_us() - start);
		if (t <= 0) {
			*timeout = 0;
			if (!done)
				return -ETIMEDOUT;
		} else {
			*timeout = t;
		}
	} while (!done);

	return 0;
}

static int wpa_request(int fd, const void *cmd, size_t cmd_len,
		       void *reply, size_t *reply_len, int64_t *t2,
		       const sigset_t *mask)
{
	char buf[REQ_REPLY_MAX];
	size_t l = REQ_REPLY_MAX;
	int64_t *t, t1 = -1, max;
	int r;

	if (!cmd || !cmd_len)
		return -EINVAL;
	if (fd < 0)
		return -ENODEV;

	if (t2)
		t = t2;
	else
		t = &t1;
	if (!reply)
		reply = buf;
	if (!reply_len)
		reply_len = &l;
	if (*reply_len < 2)
		return -EINVAL;
	*reply_len -= 1;

	/* use a maximum of 10s */
	max = 10LL * 1000LL * 1000LL;
	if (*t < 0 || *t > max)
		*t = max;

	/* send() with timeout */
	r = timed_send(fd, cmd, cmd_len, t, mask);
	if (r < 0)
		return r;

	/* recv() with timeout */
	r = timed_recv(fd, reply, reply_len, t, mask);
	if (r < 0)
		return r;
	((char*)reply)[*reply_len] = 0;

	return 0;
}

static int wpa_request_ok(int fd, const void *cmd, size_t cmd_len, int64_t *t,
			  const sigset_t *mask)
{
	char buf[REQ_REPLY_MAX];
	size_t l = REQ_REPLY_MAX;
	int r;

	r = wpa_request(fd, cmd, cmd_len, buf, &l, t, mask);
	if (r < 0)
		return r;

	if (l != 3 || strncmp(buf, "OK\n", 3))
		return -EINVAL;

	return 0;
}

_shl_public_
int wfd_wpa_ctrl_request(struct wfd_wpa_ctrl *wpa, const void *cmd,
			 size_t cmd_len, void *reply, size_t *reply_len,
			 int timeout)
{
	int64_t t;

	if (!wpa)
		return -EINVAL;
	if (!wfd_wpa_ctrl_is_open(wpa))
		return -ENODEV;

	/* prevent mult-overflow */
	if (timeout < 0)
		timeout = -1;
	else if (timeout > 1000000)
		timeout = 1000000;
	t = timeout * 1000LL;

	return wpa_request(wpa->req_fd, cmd, cmd_len, reply, reply_len, &t,
			   &wpa->mask);
}

_shl_public_
int wfd_wpa_ctrl_request_ok(struct wfd_wpa_ctrl *wpa, const void *cmd,
			    size_t cmd_len, int timeout)
{
	char buf[REQ_REPLY_MAX];
	size_t l = REQ_REPLY_MAX;
	int r;

	if (!wpa)
		return -EINVAL;

	r = wfd_wpa_ctrl_request(wpa, cmd, cmd_len, buf, &l, timeout);
	if (r < 0)
		return r;

	if (l != 3 || strncmp(buf, "OK\n", 3))
		return -EINVAL;

	return 0;
}
