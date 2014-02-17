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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libwfd.h"
#include "shl_macro.h"

_shl_public_
void wfd_wpa_event_init(struct wfd_wpa_event *ev)
{
	if (!ev)
		return;

	memset(ev, 0, sizeof(*ev));
}

_shl_public_
void wfd_wpa_event_reset(struct wfd_wpa_event *ev)
{
	if (!ev)
		return;

	free(ev->raw);

	switch (ev->type) {
	case WFD_WPA_EVENT_P2P_DEVICE_FOUND:
		free(ev->p.p2p_device_found.name);
		break;
	case WFD_WPA_EVENT_P2P_GROUP_STARTED:
		free(ev->p.p2p_group_started.ifname);
		break;
	case WFD_WPA_EVENT_P2P_PROV_DISC_SHOW_PIN:
		free(ev->p.p2p_prov_disc_show_pin.pin);
		break;
	default:
		break;
	}

	memset(ev, 0, sizeof(*ev));
}

static const struct event_type {
	const char *name;
	size_t len;
	unsigned int code;
} event_list[] = {

#define EVENT(_name, _suffix) { \
		.name = _name, \
		.len = sizeof(_name) - 1, \
		.code = WFD_WPA_EVENT_ ## _suffix \
	}

	/* MUST BE ORDERED ALPHABETICALLY FOR BINARY SEARCH! */

	EVENT("AP-STA-CONNECTED", AP_STA_CONNECTED),
	EVENT("AP-STA-DISCONNECTED", AP_STA_DISCONNECTED),
	EVENT("CTRL-EVENT-SCAN-STARTED", CTRL_EVENT_SCAN_STARTED),
	EVENT("CTRL-EVENT-TERMINATING", CTRL_EVENT_TERMINATING),
	EVENT("P2P-DEVICE-FOUND", P2P_DEVICE_FOUND),
	EVENT("P2P-DEVICE-LOST", P2P_DEVICE_LOST),
	EVENT("P2P-FIND-STOPPED", P2P_FIND_STOPPED),
	EVENT("P2P-GO-NEG-FAILURE", P2P_GO_NEG_FAILURE),
	EVENT("P2P-GO-NEG-REQUEST", P2P_GO_NEG_REQUEST),
	EVENT("P2P-GO-NEG-SUCCESS", P2P_GO_NEG_SUCCESS),
	EVENT("P2P-GROUP-FORMATION-FAILURE", P2P_GROUP_FORMATION_FAILURE),
	EVENT("P2P-GROUP-FORMATION-SUCCESS", P2P_GROUP_FORMATION_SUCCESS),
	EVENT("P2P-GROUP-REMOVED", P2P_GROUP_REMOVED),
	EVENT("P2P-GROUP-STARTED", P2P_GROUP_STARTED),
	EVENT("P2P-INVITATION-RECEIVED", P2P_INVITATION_RECEIVED),
	EVENT("P2P-INVITATION-RESULT", P2P_INVITATION_RESULT),
	EVENT("P2P-PROV-DISC-ENTER-PIN", P2P_PROV_DISC_ENTER_PIN),
	EVENT("P2P-PROV-DISC-PBC-REQ", P2P_PROV_DISC_PBC_REQ),
	EVENT("P2P-PROV-DISC-PBC-RESP", P2P_PROV_DISC_PBC_RESP),
	EVENT("P2P-PROV-DISC-SHOW-PIN", P2P_PROV_DISC_SHOW_PIN),
	EVENT("P2P-SERV-DISC-REQ", P2P_SERV_DISC_REQ),
	EVENT("P2P-SERV-DISC-RESP", P2P_SERV_DISC_RESP),

#undef EVENT
};

_shl_public_
const char *wfd_wpa_event_name(unsigned int type)
{
	size_t i, max;

	max = sizeof(event_list) / sizeof(*event_list);

	for (i = 0; i < max; ++i) {
		if (event_list[i].code == type)
			return event_list[i].name;
	}

	return "UNKNOWN";
}

static int event_comp(const void *key, const void *type)
{
	const struct event_type *t;
	const char *k;
	int r;

	k = key;
	t = type;

	r = strncmp(k, t->name, t->len);
	if (r)
		return r;

	if (k[t->len] != 0 && k[t->len] != ' ')
		return 1;

	return 0;
}

static char *tokenize(const char *src, size_t *num)
{
	char *buf, *dst;
	char last_c;
	size_t n;
	bool quoted, escaped;

	buf = malloc(strlen(src) + 1);
	if (!buf)
		return NULL;

	dst = buf;
	last_c = 0;
	n = 0;
	*dst = 0;
	quoted = 0;
	escaped = 0;

	for ( ; *src; ++src) {
		if (quoted) {
			if (escaped) {
				escaped = 0;
				last_c = *src;
				*dst++ = last_c;
			} else if (*src == '\'') {
				quoted = 0;
			} else if (*src == '\\') {
				escaped = 1;
			} else {
				last_c = *src;
				*dst++ = last_c;
			}
		} else {
			if (*src == ' ' ||
			    *src == '\n' ||
			    *src == '\t' ||
			    *src == '\r') {
				if (last_c) {
					*dst++ = 0;
					++n;
				}
				last_c = 0;
			} else if (*src == '\'') {
				quoted = 1;
				escaped = 0;
				last_c = *src;
			} else {
				last_c = *src;
				*dst++ = last_c;
			}
		}
	}

	if (last_c) {
		*dst = 0;
		++n;
	}

	*num = n;
	return buf;
}

static int parse_mac(char *buf, const char *src)
{
	int r, a1, a2, a3, a4, a5, a6;

	if (strlen(src) > 17)
		return -EINVAL;

	r = sscanf(src, "%2x:%2x:%2x:%2x:%2x:%2x",
		   &a1, &a2, &a3, &a4, &a5, &a6);
	if (r != 6)
		return -EINVAL;

	strcpy(buf, src);
	return 0;
}

static int parse_ap_sta_connected(struct wfd_wpa_event *ev,
				  char *tokens, size_t num)
{
	int r;

	if (num < 1)
		return -EINVAL;

	r = parse_mac(ev->p.ap_sta_connected.mac, tokens);
	if (r < 0)
		return r;

	return 0;
}

static int parse_ap_sta_disconnected(struct wfd_wpa_event *ev,
				     char *tokens, size_t num)
{
	int r;

	if (num < 1)
		return -EINVAL;

	r = parse_mac(ev->p.ap_sta_disconnected.mac, tokens);
	if (r < 0)
		return r;

	return 0;
}

static int parse_p2p_device_found(struct wfd_wpa_event *ev,
				  char *tokens, size_t num)
{
	int r;
	size_t i;

	if (num < 2)
		return -EINVAL;

	r = parse_mac(ev->p.p2p_device_found.peer_mac, tokens);
	if (r < 0)
		return r;

	tokens += strlen(tokens) +  1;
	for (i = 1; i < num; ++i, tokens += strlen(tokens) + 1) {
		if (strncmp(tokens, "name=", 5))
			continue;

		ev->p.p2p_device_found.name = strdup(&tokens[5]);
		if (!ev->p.p2p_device_found.name)
			return -ENOMEM;

		return 0;
	}

	return -EINVAL;
}

static int parse_p2p_device_lost(struct wfd_wpa_event *ev,
				 char *tokens, size_t num)
{
	int r;
	size_t i;

	if (num < 1)
		return -EINVAL;

	for (i = 0; i < num; ++i, tokens += strlen(tokens) + 1) {
		if (strncmp(tokens, "p2p_dev_addr=", 13))
			continue;

		r = parse_mac(ev->p.p2p_device_lost.peer_mac, &tokens[13]);
		if (r < 0)
			return r;

		return 0;
	}

	return 0;
}

static int parse_p2p_go_neg_success(struct wfd_wpa_event *ev,
				    char *tokens, size_t num)
{
	int r;
	size_t i;
	bool has_role = false, has_peer = false, has_iface = false;

	if (num < 3)
		return -EINVAL;

	for (i = 0; i < num; ++i, tokens += strlen(tokens) + 1) {
		if (!strncmp(tokens, "role=", 5)) {
			if (!strcmp(&tokens[5], "GO"))
				ev->p.p2p_go_neg_success.role = WFD_WPA_EVENT_ROLE_GO;
			else if (!strcmp(&tokens[5], "client"))
				ev->p.p2p_go_neg_success.role = WFD_WPA_EVENT_ROLE_CLIENT;
			else
				return -EINVAL;

			has_role = true;
		} else if (!strncmp(tokens, "peer_dev=", 9)) {
			r = parse_mac(ev->p.p2p_go_neg_success.peer_mac,
				      &tokens[9]);
			if (r < 0)
				return r;

			has_peer = true;
		} else if (!strncmp(tokens, "peer_iface=", 11)) {
			r = parse_mac(ev->p.p2p_go_neg_success.peer_iface,
				      &tokens[11]);
			if (r < 0)
				return r;

			has_iface = true;
		}
	}

	return (has_role && has_peer && has_iface) ? 0 : -EINVAL;
}

static int parse_p2p_group_started(struct wfd_wpa_event *ev,
				   char *tokens, size_t num)
{
	int r;
	size_t i;

	if (num < 3)
		return -EINVAL;

	ev->p.p2p_group_started.ifname = strdup(tokens);
	if (!ev->p.p2p_group_started.ifname)
		return -ENOMEM;

	tokens += strlen(tokens) + 1;

	if (!strcmp(tokens, "GO"))
		ev->p.p2p_group_started.role = WFD_WPA_EVENT_ROLE_GO;
	else if (!strcmp(tokens, "client"))
		ev->p.p2p_group_started.role = WFD_WPA_EVENT_ROLE_CLIENT;
	else
		return -EINVAL;

	tokens += strlen(tokens) + 1;

	for (i = 2; i < num; ++i, tokens += strlen(tokens) + 1) {
		if (strncmp(tokens, "go_dev_addr=", 12))
			continue;

		r = parse_mac(ev->p.p2p_group_started.go_mac, &tokens[12]);
		if (r < 0)
			return r;

		return 0;
	}

	return -EINVAL;
}

static int parse_p2p_group_removed(struct wfd_wpa_event *ev,
				   char *tokens, size_t num)
{
	if (num < 2)
		return -EINVAL;

	ev->p.p2p_group_removed.ifname = strdup(tokens);
	if (!ev->p.p2p_group_removed.ifname)
		return -ENOMEM;

	tokens += strlen(tokens) + 1;

	if (!strcmp(tokens, "GO"))
		ev->p.p2p_group_removed.role = WFD_WPA_EVENT_ROLE_GO;
	else if (!strcmp(tokens, "client"))
		ev->p.p2p_group_removed.role = WFD_WPA_EVENT_ROLE_CLIENT;
	else
		return -EINVAL;

	return 0;
}

static int parse_p2p_prov_disc_show_pin(struct wfd_wpa_event *ev,
					char *tokens, size_t num)
{
	int r;

	if (num < 2)
		return -EINVAL;

	r = parse_mac(ev->p.p2p_prov_disc_show_pin.peer_mac, tokens);
	if (r < 0)
		return r;

	tokens += strlen(tokens) +  1;
	ev->p.p2p_prov_disc_show_pin.pin = strdup(tokens);
	if (!ev->p.p2p_prov_disc_show_pin.pin)
		return -ENOMEM;

	return 0;
}

static int parse_p2p_prov_disc_enter_pin(struct wfd_wpa_event *ev,
					 char *tokens, size_t num)
{
	int r;

	if (num < 1)
		return -EINVAL;

	r = parse_mac(ev->p.p2p_prov_disc_enter_pin.peer_mac, tokens);
	if (r < 0)
		return r;

	return 0;
}

static int parse_p2p_prov_disc_pbc_req(struct wfd_wpa_event *ev,
				       char *tokens, size_t num)
{
	int r;

	if (num < 1)
		return -EINVAL;

	r = parse_mac(ev->p.p2p_prov_disc_pbc_req.peer_mac, tokens);
	if (r < 0)
		return r;

	return 0;
}

static int parse_p2p_prov_disc_pbc_resp(struct wfd_wpa_event *ev,
					char *tokens, size_t num)
{
	int r;

	if (num < 1)
		return -EINVAL;

	r = parse_mac(ev->p.p2p_prov_disc_pbc_resp.peer_mac, tokens);
	if (r < 0)
		return r;

	return 0;
}

_shl_public_
int wfd_wpa_event_parse(struct wfd_wpa_event *ev, const char *event)
{
	const char *t;
	char *end, *tokens = NULL;
	size_t num;
	struct event_type *code;
	int r;

	if (!ev || !event)
		return -EINVAL;

	wfd_wpa_event_reset(ev);

	if (*event == '<') {
		t = strchr(event, '>');
		if (!t)
			goto unknown;

		++t;
		ev->priority = strtoul(event + 1, &end, 10);
		if (ev->priority >= WFD_WPA_EVENT_P_CNT ||
		    end + 1 != t ||
		    event[1] == '+' ||
		    event[1] == '-')
			ev->priority = WFD_WPA_EVENT_P_MSGDUMP;
	} else {
		t = event;
		ev->priority = WFD_WPA_EVENT_P_MSGDUMP;
	}

	code = bsearch(t, event_list,
		       sizeof(event_list) / sizeof(*event_list),
		       sizeof(*event_list),
		       event_comp);
	if (!code)
		goto unknown;

	ev->type = code->code;
	t += code->len;
	while (*t == ' ')
		++t;

	ev->raw = strdup(t);
	if (!ev->raw) {
		r = -ENOMEM;
		goto error;
	}

	tokens = tokenize(ev->raw, &num);
	if (!tokens) {
		r = -ENOMEM;
		goto error;
	}

	switch (ev->type) {
	case WFD_WPA_EVENT_AP_STA_CONNECTED:
		r = parse_ap_sta_connected(ev, tokens, num);
		break;
	case WFD_WPA_EVENT_AP_STA_DISCONNECTED:
		r = parse_ap_sta_disconnected(ev, tokens, num);
		break;
	case WFD_WPA_EVENT_P2P_DEVICE_FOUND:
		r = parse_p2p_device_found(ev, tokens, num);
		break;
	case WFD_WPA_EVENT_P2P_DEVICE_LOST:
		r = parse_p2p_device_lost(ev, tokens, num);
		break;
	case WFD_WPA_EVENT_P2P_GO_NEG_SUCCESS:
		r = parse_p2p_go_neg_success(ev, tokens, num);
		break;
	case WFD_WPA_EVENT_P2P_GROUP_STARTED:
		r = parse_p2p_group_started(ev, tokens, num);
		break;
	case WFD_WPA_EVENT_P2P_GROUP_REMOVED:
		r = parse_p2p_group_removed(ev, tokens, num);
		break;
	case WFD_WPA_EVENT_P2P_PROV_DISC_SHOW_PIN:
		r = parse_p2p_prov_disc_show_pin(ev, tokens, num);
		break;
	case WFD_WPA_EVENT_P2P_PROV_DISC_ENTER_PIN:
		r = parse_p2p_prov_disc_enter_pin(ev, tokens, num);
		break;
	case WFD_WPA_EVENT_P2P_PROV_DISC_PBC_REQ:
		r = parse_p2p_prov_disc_pbc_req(ev, tokens, num);
		break;
	case WFD_WPA_EVENT_P2P_PROV_DISC_PBC_RESP:
		r = parse_p2p_prov_disc_pbc_resp(ev, tokens, num);
		break;
	default:
		r = 0;
		break;
	}

	free(tokens);

	if (r < 0)
		goto error;

	return 0;

unknown:
	ev->type = WFD_WPA_EVENT_UNKNOWN;
	return 0;

error:
	wfd_wpa_event_reset(ev);
	return r;
}
