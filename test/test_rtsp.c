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

#include "test_common.h"

static int received;

struct orig {
	ssize_t len;
	const char *str;
};

struct expect {
	size_t times;
	struct wfd_rtsp_msg msg;
};

static const struct orig orig[] = {
	{ -1, "SOMETHING\r\n\r\n" },
	{ -1, "SOMETHING" },
	{ -1, "\r" },
	{ -1, "\n" },
	{ -1, "\n" },
	{ -1, "SOME" },
	{ -1, "THING" },
	{ -1, "\r" },
	{ -1, "\r" },
	{ -1, "\n" },
	{ -1, "SOME" },
	{ -1, "THING\n" },
	{ -1, "\r" },
	{ -1, "SOMETHING\n\n" },
	{ -1, "SOMETHING\r\r" },
	{ -1, "SOMETHING\r\r\n" },
	{ -1, "SOMETHING\n\r\n" },

	{ -1, "OPTIONS * RTSP/1.0\n\r\n" },
	{ -1, "OPTIONS    *    RTSP/1.0\n\r\n" },
	{ -1, "OPTIONS *\r RTSP/1.0\n\r\n" },
	{ -1, "OPTIONS *\r\n RTSP/1.0\n\r\n" },
	{ -1, "OPTIONS\r *\n RTSP/1.0\n\r\n" },
	{ -1, "  \r\n   OPTIONS * RTSP/1.0\n\r\n" },
	{ -1, "\rOPTIONS * RTSP/1.0\n\r\n" },
	{ -1, "\nOPTIONS * RTSP/1.0\n\r\n" },
	{ -1, " OPTIONS *\n\t \r\tRTSP/1.0\n\r\n" },
	{ -1, "OPTIONS * RTSP/1.0   \n\r\n" },

	{ -1, "RTSP/1.0 200 OK Something\n\n" },

	{ 10, "$\001\000\006RAWSTH" },
	{ 10, "$\001\000\006RAWSTH" },

	{ -1, "SOMETHING\r\nsome-header:value\r\n\r\n" },

	{ -1, "OPTIONS * RTSP/2.1\n" },
	{ -1, "some-header:value\n" },
	{ -1, "some-other-header:buhu\n" },
	{ -1, "\n" },
	{ -1, "OPTIONS * RTSP/2.1\n" },
	{ -1, "some-header:value \n" },
	{ -1, "some-other-header:buhu \r \n \n" },
	{ -1, "\n" },

	{ 16, "  \n   $\001\000\006RAWSTH" },
	{ 11, "  \n   \r\n$\001\000" },
	{ 7, "\006RAWSTH" },

	{ -1, "OPTIONS * RTSP/2.1\n" },
	{ -1, "some-header :value \n" },
	{ -1, "some-other-header: buhu \r \n \n" },
	{ -1, "some-header : value \n" },
	{ -1, "\n" },
	{ -1, "OPTIONS * RTSP/2.1\n" },
	{ -1, "some-header  \r   \n :value \n" },
	{ -1, "some-other-header: \r\n buhu \r \n \n" },
	{ -1, "some-header        \t\t\t:\r\n value    \n" },
	{ -1, "\n" },

	{ -1, "STH\r\ncontent-length:5\r\n\r\n12345" },

	{ -1, "STH\r\ncontent-length:5/suffix\r\n\r\n12345" },

	{ -1, "OPTIONS * RTSP/1.0\n" },
	{ -1, "cseq: 100\n" },
	{ -1, "\n" },

	/* leave this at the end to test missing trailing \n */
	{ -1, "SOMETHING\n\r" },
};

static const struct expect expect[] = {
	{
		.times = 8,
		.msg = {
			.type = WFD_RTSP_MSG_UNKNOWN,
			.id = {
				/* test .length comparison in test-module */
				.line = "SOMETHING" "-STUPID",
				.length = 9,
			},
		},
	},
	{
		.times = 10,
		.msg = {
			.type = WFD_RTSP_MSG_REQUEST,
			.id = {
				.line = "OPTIONS * RTSP/1.0",
				.request = {
					.method = "OPTIONS",
					.type = WFD_RTSP_METHOD_OPTIONS,
					.uri = "*",
					.major = 1,
					.minor = 0,
				},
			},
		},
	},
	{
		.times = 1,
		.msg = {
			.type = WFD_RTSP_MSG_RESPONSE,
			.id = {
				.line = "RTSP/1.0 200 OK Something",
				.response = {
					.major = 1,
					.minor = 0,
					.status = 200,
					.phrase = "OK Something",
				},
			},
		},
	},
	{
		.times = 1,
		.msg = {
			.type = WFD_RTSP_MSG_UNKNOWN,
			.id = {
				.line = "SOMETHING",
			},
			.headers = {
				[WFD_RTSP_HEADER_UNKNOWN] = {
					.count = 1,
					.lines = (char*[]){
						(char*)"some-header:value",
					},
					.lengths = (size_t[]){
						0,
					},
				},
			},
		},
	},
	{
		.times = 2,
		.msg = {
			.type = WFD_RTSP_MSG_REQUEST,
			.id = {
				.line = "OPTIONS * RTSP/2.1",
				.request = {
					.method = "OPTIONS",
					.type = WFD_RTSP_METHOD_OPTIONS,
					.uri = "*",
					.major = 2,
					.minor = 1,
				},
			},
			.headers = {
				[WFD_RTSP_HEADER_UNKNOWN] = {
					.count = 2,
					.lines = (char*[]){
						(char*)"some-header:value",
						(char*)"some-other-header:buhu",
					},
					.lengths = (size_t[]){
						0,
						0,
					},
				},
			},
		},
	},
	{
		.times = 2,
		.msg = {
			.type = WFD_RTSP_MSG_REQUEST,
			.id = {
				.line = "OPTIONS * RTSP/2.1",
				.request = {
					.method = "OPTIONS",
					.type = WFD_RTSP_METHOD_OPTIONS,
					.uri = "*",
					.major = 2,
					.minor = 1,
				},
			},
			.headers = {
				[WFD_RTSP_HEADER_UNKNOWN] = {
					.count = 3,
					.lines = (char*[]){
						(char*)"some-header :value",
						(char*)"some-other-header: buhu",
						(char*)"some-header : value",
					},
					.lengths = (size_t[]){
						0,
						0,
						0,
					},
				},
			},
		},
	},
	{
		.times = 1,
		.msg = {
			.type = WFD_RTSP_MSG_UNKNOWN,
			.id = {
				.line = "STH",
			},
			.headers = {
				[WFD_RTSP_HEADER_CONTENT_LENGTH] = {
					.count = 1,
					.lines = (char*[]){
						(char*)"content-length:5",
					},
					.lengths = (size_t[]){
						0,
					},
				},
			},
			.entity = {
				.value = "12345",
			},
		},
	},
	{
		.times = 1,
		.msg = {
			.type = WFD_RTSP_MSG_UNKNOWN,
			.id = {
				.line = "STH",
			},
			.headers = {
				[WFD_RTSP_HEADER_CONTENT_LENGTH] = {
					.count = 1,
					.lines = (char*[]){
						(char*)"content-length:5/suffix",
					},
					.lengths = (size_t[]){
						0,
					},
				},
			},
			.entity = {
				.value = "12345",
			},
		},
	},
	{
		.times = 1,
		.msg = {
			.type = WFD_RTSP_MSG_REQUEST,
			.id = {
				.line = "OPTIONS * RTSP/1.0",
				.request = {
					.method = "OPTIONS",
					.type = WFD_RTSP_METHOD_OPTIONS,
					.uri = "*",
					.major = 1,
					.minor = 0,
				},
			},
			.headers = {
				[WFD_RTSP_HEADER_CSEQ] = {
					.count = 1,
					.lines = (char*[]){
						(char*)"cseq: 100",
					},
					.lengths = (size_t[]){
						0,
					},
				},
			},
		},
	},
	{
		.times = 1,
		.msg = {
			.type = WFD_RTSP_MSG_UNKNOWN,
			.id = {
				.line = "SOMETHING",
			},
		},
	},
};

#define LEN(_len, _str) ((_len) <= 0 ? ((_str) ? strlen(_str) : 0) : (_len))

static int test_wfd_rtsp_decoder_event(struct wfd_rtsp_decoder *dec,
				   void *data,
				   struct wfd_rtsp_decoder_event *ev)
{
	bool debug = true;
	static int pos, num;
	size_t i, j;
	const struct expect *e;
	const struct wfd_rtsp_msg *m, *msg;
	const struct wfd_rtsp_msg_header *h, *hm;

	if (ev->type == WFD_RTSP_DECODER_DATA) {
		if (debug)
			fprintf(stderr, "Raw Data (%u:%u): %s\n",
				(unsigned int)ev->data.channel,
				(unsigned int)ev->data.size,
				(char*)ev->data.value);
		return 0;
	}

	if (ev->type != WFD_RTSP_DECODER_MSG)
		return 0;

	msg = ev->msg;
	ck_assert(data == TEST_INVALID_PTR);
	++received;

	ck_assert(pos < SHL_ARRAY_LENGTH(expect));
	e = &expect[pos];
	m = &e->msg;
	if (++num >= e->times) {
		++pos;
		num = 0;
	}

	/* print msg */

	if (debug) {
		fprintf(stderr, "Received Message:\n");
		fprintf(stderr, "  type: %u\n", msg->type);
		fprintf(stderr, "  id: len: %zu line: %s\n",
			msg->id.length, msg->id.line);

		if (msg->type == WFD_RTSP_MSG_REQUEST) {
			fprintf(stderr, "  method (%d): %s %s RTSP/%u.%u\n",
				msg->id.request.type,
				msg->id.request.method,
				msg->id.request.uri,
				msg->id.request.major,
				msg->id.request.minor);
		} else if (msg->type == WFD_RTSP_MSG_RESPONSE) {
			fprintf(stderr, "  RTSP/%u.%u %u %s\n",
				msg->id.response.major,
				msg->id.response.minor,
				msg->id.response.status,
				msg->id.response.phrase);
		}

		fprintf(stderr, "  headers:\n");
		for (i = 0; i < SHL_ARRAY_LENGTH(msg->headers); ++i) {
			h = &msg->headers[i];
			if (!h->count)
				continue;

			fprintf(stderr, "    id: %zu %s (count: %zu)\n",
				i, wfd_rtsp_header_get_name(i) ? : "<unknown>",
				h->count);

			for (j = 0; j < h->count; ++j) {
				fprintf(stderr, "      line (%zu): %s\n",
					h->lengths[j], h->lines[j]);
			}
		}

		if (msg->entity.size)
			fprintf(stderr, "  body (%zu): %s\n",
				msg->entity.size, (char*)msg->entity.value);
	}

	/* compare msgs */

	ck_assert_int_eq(m->type, msg->type);
	ck_assert_int_eq(LEN(m->id.length, m->id.line), msg->id.length);
	ck_assert(!memcmp(m->id.line, msg->id.line, msg->id.length));

	if (m->type == WFD_RTSP_MSG_REQUEST) {
		ck_assert(!strcmp(m->id.request.method,
				  msg->id.request.method));
		ck_assert_int_eq(m->id.request.type,
				 msg->id.request.type);
		ck_assert(!strcmp(m->id.request.uri,
				  msg->id.request.uri));
		ck_assert_int_eq(m->id.request.major,
				 msg->id.request.major);
		ck_assert_int_eq(m->id.request.minor,
				 msg->id.request.minor);
	} else if (m->type == WFD_RTSP_MSG_RESPONSE) {
		ck_assert_int_eq(m->id.response.major,
				 msg->id.response.major);
		ck_assert_int_eq(m->id.response.minor,
				 msg->id.response.minor);
		ck_assert_int_eq(m->id.response.status,
				 msg->id.response.status);
		ck_assert(!strcmp(m->id.response.phrase,
				  msg->id.response.phrase));
	}

	for (i = 0; i < SHL_ARRAY_LENGTH(msg->headers); ++i) {
		h = &m->headers[i];
		hm = &msg->headers[i];
		ck_assert_int_eq(h->count, hm->count);

		for (j = 0; j < h->count; ++j) {
			ck_assert_int_eq(LEN(h->lengths[j], h->lines[j]),
					 hm->lengths[j]);
			ck_assert(!memcmp(h->lines[j],
					  hm->lines[j],
					  hm->lengths[j]));
		}
	}

	ck_assert_int_eq(LEN(m->entity.size, m->entity.value),
			 msg->entity.size);
	ck_assert(!memcmp(m->entity.value, msg->entity.value,
			  msg->entity.size));

	return 0;
}

START_TEST(test_wfd_rtsp_decoder)
{
	struct wfd_rtsp_decoder *d;
	int r, sent = 0;
	size_t len, num, i;

	r = wfd_rtsp_decoder_new(NULL, NULL, NULL, NULL, &d);
	ck_assert(r == -EINVAL);

	r = wfd_rtsp_decoder_new(test_wfd_rtsp_decoder_event, NULL, NULL, NULL, &d);
	ck_assert(r >= 0);

	wfd_rtsp_decoder_set_data(d, TEST_INVALID_PTR);
	ck_assert(wfd_rtsp_decoder_get_data(d) == TEST_INVALID_PTR);
	ck_assert(received == sent);

	for (i = 0; i < SHL_ARRAY_LENGTH(orig); ++i) {
		if (orig[i].len >= 0)
			len = orig[i].len;
		else
			len = strlen(orig[i].str);

		r = wfd_rtsp_decoder_feed(d, orig[i].str, len);
		ck_assert(r >= 0);
	}

	num = 0;
	for (i = 0; i < SHL_ARRAY_LENGTH(expect); ++i)
		num += expect[i].times;

	ck_assert_int_eq(received, num);

	wfd_rtsp_decoder_free(d);
}
END_TEST

static void tokenize(const char *line,
		     size_t linelen,
		     const char *expect,
		     size_t len,
		     size_t num)
{
	const char *s;
	char *t;
	ssize_t l, i;

	ck_assert(len > 0);

	l = wfd_rtsp_tokenize(line, linelen, &t);
	ck_assert(l >= 0);

	if (0) {
		fprintf(stderr, "TOKENIZER (%lu):\n", (unsigned long)l);
		s = t;
		for (i = 0; i < l; ++i) {
			fprintf(stderr, "  TOKEN: %s\n", s);
			s += strlen(s) + 1;
		}

		fprintf(stderr, "EXPECT (%lu):\n", (unsigned long)num);
		s = expect;
		for (i = 0; i < l; ++i) {
			fprintf(stderr, "  TOKEN: %s\n", s);
			s += strlen(s) + 1;
		}
	}

	ck_assert(l == (ssize_t)num);
	ck_assert(!memcmp(t, expect, len));
	free(t);
}

#define TOKENIZE(_line, _exp, _num) \
	tokenize((_line), strlen(_line), (_exp), sizeof(_exp), _num)
#define TOKENIZE_N(_line, _len, _exp, _num) \
	tokenize((_line), _len, (_exp), sizeof(_exp), _num)

START_TEST(test_wfd_rtsp_tokenizer)
{
	TOKENIZE("", "", 0);
	TOKENIZE("asdf", "asdf", 1);
	TOKENIZE("asdf\"\"asdf", "asdf\0\0asdf", 3);
	TOKENIZE("asdf\"asdf\"asdf", "asdf\0asdf\0asdf", 3);
	TOKENIZE("\"asdf\"", "asdf", 1);
	TOKENIZE("\"\\n\\\\\\r\"", "\n\\\r", 1);
	TOKENIZE("\"\\\"\"", "\"", 1);
	TOKENIZE("\"\\0\"", "\\0", 1);
	TOKENIZE("\"\\\0\"", "\\", 1);
	TOKENIZE_N("\"\\\0\"", 4, "\\0", 1);
	TOKENIZE("\"\\0\\\0\"", "\\0\\", 1);
	TOKENIZE_N("\"\\0\\\0\"", 6, "\\0\\0", 1);
	TOKENIZE("content-length:   100", "content-length\0:\0""100", 3);
	TOKENIZE("content-args: (50+10)", "content-args\0:\0(\0""50+10\0)", 5);
	TOKENIZE("content-args: (50 + 10)", "content-args\0:\0(\0""50\0+\0""10\0)", 7);
}
END_TEST

TEST_DEFINE_CASE(decoder)
	TEST(test_wfd_rtsp_decoder)
	TEST(test_wfd_rtsp_tokenizer)
TEST_END_CASE

TEST_DEFINE(
	TEST_SUITE(rtsp,
		TEST_CASE(decoder),
		TEST_END
	)
)
