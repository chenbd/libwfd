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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "libwfd.h"
#include "shl_llog.h"
#include "shl_macro.h"
#include "shl_ring.h"
#include "shl_util.h"

enum state {
	STATE_NEW,
	STATE_HEADER,
	STATE_HEADER_QUOTE,
	STATE_HEADER_NL,
	STATE_BODY,
	STATE_DATA_HEAD,
	STATE_DATA_BODY,
};

struct wfd_rtsp_decoder {
	wfd_rtsp_decoder_event_t event_fn;
	void *data;
	llog_submit_t llog;
	void *llog_data;

	struct wfd_rtsp_msg msg;

	struct shl_ring buf;
	size_t buflen;
	unsigned int state;
	char last_chr;
	size_t remaining_body;

	uint8_t data_channel;
	size_t data_size;

	bool quoted : 1;
	bool dead : 1;
};

/*
 * Lookup Tables
 */

static const char *method_names[] = {
	[WFD_RTSP_METHOD_ANNOUNCE]		= "ANNOUNCE",
	[WFD_RTSP_METHOD_DESCRIBE]		= "DESCRIBE",
	[WFD_RTSP_METHOD_GET_PARAMETER]	= "GET_PARAMETER",
	[WFD_RTSP_METHOD_OPTIONS]		= "OPTIONS",
	[WFD_RTSP_METHOD_PAUSE]		= "PAUSE",
	[WFD_RTSP_METHOD_PLAY]		= "PLAY",
	[WFD_RTSP_METHOD_RECORD]		= "RECORD",
	[WFD_RTSP_METHOD_REDIRECT]		= "REDIRECT",
	[WFD_RTSP_METHOD_SETUP]		= "SETUP",
	[WFD_RTSP_METHOD_SET_PARAMETER]	= "SET_PARAMETER",
	[WFD_RTSP_METHOD_TEARDOWN]		= "TEARDOWN",
	[WFD_RTSP_METHOD_CNT]		= NULL,
};

_shl_public_
const char *wfd_rtsp_method_get_name(unsigned int method)
{
	if (method >= SHL_ARRAY_LENGTH(method_names))
		return NULL;

	return method_names[method];
}

_shl_public_
unsigned int wfd_rtsp_method_from_name(const char *method)
{
	size_t i;

	for (i = 0; i < SHL_ARRAY_LENGTH(method_names); ++i)
		if (method_names[i] && !strcasecmp(method, method_names[i]))
			return i;

	return WFD_RTSP_METHOD_UNKNOWN;
}

_shl_public_
bool wfd_rtsp_status_is_valid(unsigned int status)
{
	return status >= 100 && status < 600;
}

_shl_public_
unsigned int wfd_rtsp_status_get_base(unsigned int status)
{
	switch (status) {
	case 100 ... 199:
		return 100;
	case 200 ... 299:
		return 200;
	case 300 ... 399:
		return 300;
	case 400 ... 499:
		return 400;
	case 500 ... 599:
		return 500;
	default:
		return 600;
	}
}

static const char *status_descriptions[] = {
	[WFD_RTSP_STATUS_CONTINUE]					= "Continue",

	[WFD_RTSP_STATUS_OK]					= "OK",
	[WFD_RTSP_STATUS_CREATED]					= "Created",

	[WFD_RTSP_STATUS_LOW_ON_STORAGE_SPACE]			= "Low on Storage Space",

	[WFD_RTSP_STATUS_MULTIPLE_CHOICES]				= "Multiple Choices",
	[WFD_RTSP_STATUS_MOVED_PERMANENTLY]				= "Moved Permanently",
	[WFD_RTSP_STATUS_MOVED_TEMPORARILY]				= "Moved Temporarily",
	[WFD_RTSP_STATUS_SEE_OTHER]					= "See Other",
	[WFD_RTSP_STATUS_NOT_MODIFIED]				= "Not Modified",
	[WFD_RTSP_STATUS_USE_PROXY]					= "Use Proxy",

	[WFD_RTSP_STATUS_BAD_REQUEST]				= "Bad Request",
	[WFD_RTSP_STATUS_UNAUTHORIZED]				= "Unauthorized",
	[WFD_RTSP_STATUS_PAYMENT_REQUIRED]				= "Payment Required",
	[WFD_RTSP_STATUS_FORBIDDEN]					= "Forbidden",
	[WFD_RTSP_STATUS_NOT_FOUND]					= "Not Found",
	[WFD_RTSP_STATUS_METHOD_NOT_ALLOWED]			= "Method not Allowed",
	[WFD_RTSP_STATUS_NOT_ACCEPTABLE]				= "Not Acceptable",
	[WFD_RTSP_STATUS_PROXY_AUTHENTICATION_REQUIRED]		= "Proxy Authentication Required",
	[WFD_RTSP_STATUS_REQUEST_TIMEOUT]				= "Request Time-out",
	[WFD_RTSP_STATUS_GONE]					= "Gone",
	[WFD_RTSP_STATUS_LENGTH_REQUIRED]				= "Length Required",
	[WFD_RTSP_STATUS_PRECONDITION_FAILED]			= "Precondition Failed",
	[WFD_RTSP_STATUS_REQUEST_ENTITY_TOO_LARGE]			= "Request Entity Too Large",
	[WFD_RTSP_STATUS_REQUEST_URI_TOO_LARGE]			= "Request-URI too Large",
	[WFD_RTSP_STATUS_UNSUPPORTED_MEDIA_TYPE]			= "Unsupported Media Type",

	[WFD_RTSP_STATUS_PARAMETER_NOT_UNDERSTOOD]			= "Parameter not Understood",
	[WFD_RTSP_STATUS_CONFERENCE_NOT_FOUND]			= "Conference not Found",
	[WFD_RTSP_STATUS_NOT_ENOUGH_BANDWIDTH]			= "Not Enough Bandwidth",
	[WFD_RTSP_STATUS_SESSION_NOT_FOUND]				= "Session not Found",
	[WFD_RTSP_STATUS_METHOD_NOT_VALID_IN_THIS_STATE]		= "Method not Valid in this State",
	[WFD_RTSP_STATUS_HEADER_FIELD_NOT_VALID_FOR_RESOURCE]	= "Header Field not Valid for Resource",
	[WFD_RTSP_STATUS_INVALID_RANGE]				= "Invalid Range",
	[WFD_RTSP_STATUS_PARAMETER_IS_READ_ONLY]			= "Parameter is Read-only",
	[WFD_RTSP_STATUS_AGGREGATE_OPERATION_NOT_ALLOWED]		= "Aggregate Operation not Allowed",
	[WFD_RTSP_STATUS_ONLY_AGGREGATE_OPERATION_ALLOWED]		= "Only Aggregate Operation Allowed",
	[WFD_RTSP_STATUS_UNSUPPORTED_TRANSPORT]			= "Unsupported Transport",
	[WFD_RTSP_STATUS_DESTINATION_UNREACHABLE]			= "Destination Unreachable",

	[WFD_RTSP_STATUS_INTERNAL_SERVER_ERROR]			= "Internal Server Error",
	[WFD_RTSP_STATUS_NOT_IMPLEMENTED]				= "Not Implemented",
	[WFD_RTSP_STATUS_BAD_GATEWAY]				= "Bad Gateway",
	[WFD_RTSP_STATUS_SERVICE_UNAVAILABLE]			= "Service Unavailable",
	[WFD_RTSP_STATUS_GATEWAY_TIMEOUT]				= "Gateway Time-out",
	[WFD_RTSP_STATUS_WFD_RTSP_VERSION_NOT_SUPPORTED]		= "RTSP Version not Supported",

	[WFD_RTSP_STATUS_OPTION_NOT_SUPPORTED]			= "Option not Supported",

	[WFD_RTSP_STATUS_CNT]					= NULL,
};

_shl_public_
const char *wfd_rtsp_status_get_description(unsigned int status)
{
	if (status >= SHL_ARRAY_LENGTH(status_descriptions))
		return NULL;

	return status_descriptions[status];
}

static const char *header_names[] = {
	[WFD_RTSP_HEADER_ACCEPT]			= "Accept",
	[WFD_RTSP_HEADER_ACCEPT_ENCODING]		= "Accept-Encoding",
	[WFD_RTSP_HEADER_ACCEPT_LANGUAGE]		= "Accept-Language",
	[WFD_RTSP_HEADER_ALLOW]				= "Allow",
	[WFD_RTSP_HEADER_AUTHORIZATION]			= "Authorization",
	[WFD_RTSP_HEADER_BANDWIDTH]			= "Bandwidth",
	[WFD_RTSP_HEADER_BLOCKSIZE]			= "Blocksize",
	[WFD_RTSP_HEADER_CACHE_CONTROL]			= "Cache-Control",
	[WFD_RTSP_HEADER_CONFERENCE]			= "Conference",
	[WFD_RTSP_HEADER_CONNECTION]			= "Connection",
	[WFD_RTSP_HEADER_CONTENT_BASE]			= "Content-Base",
	[WFD_RTSP_HEADER_CONTENT_ENCODING]		= "Content-Encoding",
	[WFD_RTSP_HEADER_CONTENT_LANGUAGE]		= "Content-Language",
	[WFD_RTSP_HEADER_CONTENT_LENGTH]		= "Content-Length",
	[WFD_RTSP_HEADER_CONTENT_LOCATION]		= "Content-Location",
	[WFD_RTSP_HEADER_CONTENT_TYPE]			= "Content-Type",
	[WFD_RTSP_HEADER_CSEQ]				= "CSeq",
	[WFD_RTSP_HEADER_DATE]				= "Date",
	[WFD_RTSP_HEADER_EXPIRES]			= "Expires",
	[WFD_RTSP_HEADER_FROM]				= "From",
	[WFD_RTSP_HEADER_HOST]				= "Host",
	[WFD_RTSP_HEADER_IF_MATCH]			= "If-Match",
	[WFD_RTSP_HEADER_IF_MODIFIED_SINCE]		= "If-Modified-Since",
	[WFD_RTSP_HEADER_LAST_MODIFIED]			= "Last-Modified",
	[WFD_RTSP_HEADER_LOCATION]			= "Location",
	[WFD_RTSP_HEADER_PROXY_AUTHENTICATE]		= "Proxy-Authenticate",
	[WFD_RTSP_HEADER_PROXY_REQUIRE]			= "Proxy-Require",
	[WFD_RTSP_HEADER_PUBLIC]			= "Public",
	[WFD_RTSP_HEADER_RANGE]				= "Range",
	[WFD_RTSP_HEADER_REFERER]			= "Referer",
	[WFD_RTSP_HEADER_RETRY_AFTER]			= "Retry-After",
	[WFD_RTSP_HEADER_REQUIRE]			= "Require",
	[WFD_RTSP_HEADER_RTP_INFO]			= "RTP-Info",
	[WFD_RTSP_HEADER_SCALE]				= "Scale",
	[WFD_RTSP_HEADER_SPEED]				= "Speed",
	[WFD_RTSP_HEADER_SERVER]			= "Server",
	[WFD_RTSP_HEADER_SESSION]			= "Session",
	[WFD_RTSP_HEADER_TIMESTAMP]			= "Timestamp",
	[WFD_RTSP_HEADER_TRANSPORT]			= "Transport",
	[WFD_RTSP_HEADER_UNSUPPORTED]			= "Unsupported",
	[WFD_RTSP_HEADER_USER_AGENT]			= "User-Agent",
	[WFD_RTSP_HEADER_VARY]				= "Vary",
	[WFD_RTSP_HEADER_VIA]				= "Via",
	[WFD_RTSP_HEADER_WWW_AUTHENTICATE]		= "WWW-Authenticate",
	[WFD_RTSP_HEADER_CNT]				= NULL,
};

_shl_public_
const char *wfd_rtsp_header_get_name(unsigned int header)
{
	if (header >= SHL_ARRAY_LENGTH(header_names))
		return NULL;

	return header_names[header];
}

_shl_public_
unsigned int wfd_rtsp_header_from_name(const char *header)
{
	size_t i;

	for (i = 0; i < SHL_ARRAY_LENGTH(header_names); ++i)
		if (header_names[i] && !strcasecmp(header, header_names[i]))
			return i;

	return WFD_RTSP_HEADER_UNKNOWN;
}

_shl_public_
unsigned int wfd_rtsp_header_from_name_n(const char *header, size_t len)
{
	size_t i;

	for (i = 0; i < SHL_ARRAY_LENGTH(header_names); ++i)
		if (header_names[i] &&
		    !strncasecmp(header, header_names[i], len) &&
		    !header_names[i][len])
			return i;

	return WFD_RTSP_HEADER_UNKNOWN;
}

/*
 * Helpers
 */

static void msg_clear_id(struct wfd_rtsp_msg *msg)
{
	switch (msg->type) {
	case WFD_RTSP_MSG_REQUEST:
		free(msg->id.request.method);
		free(msg->id.request.uri);
		break;
	case WFD_RTSP_MSG_RESPONSE:
		free(msg->id.response.phrase);
		break;
	}

	free(msg->id.line);
	shl_zero(msg->id);
}

static void msg_clear_headers(struct wfd_rtsp_msg *msg)
{
	struct wfd_rtsp_msg_header *h;
	size_t i, j;

	for (i = 0; i < WFD_RTSP_HEADER_CNT; ++i) {
		h = &msg->headers[i];

		for (j = 0; j < h->count; ++j)
			free(h->lines[j]);

		free(h->lines);
		free(h->lengths);
	}

	shl_zero(msg->headers);
}

static void msg_clear_entity(struct wfd_rtsp_msg *msg)
{
	free(msg->entity.value);
	shl_zero(msg->entity);
}

static void msg_clear(struct wfd_rtsp_msg *msg)
{
	msg_clear_id(msg);
	msg_clear_headers(msg);
	msg_clear_entity(msg);
}

static int decoder_call(struct wfd_rtsp_decoder *dec,
			struct wfd_rtsp_decoder_event *ev)
{
	return dec->event_fn(dec, dec->data, ev);
}

static int decoder_submit(struct wfd_rtsp_decoder *dec)
{
	struct wfd_rtsp_decoder_event ev = { };
	int r;

	ev.type = WFD_RTSP_DECODER_MSG;
	ev.msg = &dec->msg;
	r = decoder_call(dec, &ev);
	msg_clear(&dec->msg);

	return r;
}

static int decoder_submit_data(struct wfd_rtsp_decoder *dec, uint8_t *p)
{
	struct wfd_rtsp_decoder_event ev = { };

	ev.type = WFD_RTSP_DECODER_DATA;
	ev.data.channel = dec->data_channel;
	ev.data.size = dec->data_size;
	ev.data.value = p;
	return decoder_call(dec, &ev);
}

/*
 * Header ID-line Handling
 * This parses both, the REQUEST and RESPONSE lines of an RTSP method. It is
 * always the first header line and defines the type of message. If it is
 * unrecognized, we set it to UNKNOWN.
 * Note that regardless of the ID-line, all following lines are parsed as
 * generic headers followed by an optional entity.
 */

static int decoder_parse_request(struct wfd_rtsp_decoder *dec,
				 char *line,
				 size_t len)
{
	unsigned int major, minor;
	size_t cmdlen, urllen;
	char *next, *prev, *cmd, *url;

	/* Requests look like this:
	 *   <cmd> <url> RTSP/<major>.<minor>
	 * We try to match <cmd> here, but accept invalid commands. <url> is
	 * never parsed (it can become pretty complex if done properly). */

	next = line;

	/* parse <cmd> */
	cmd = line;
	next = strchr(next, ' ');
	if (!next || next == cmd)
		goto error;
	cmdlen = next - cmd;

	/* skip " " */
	++next;

	/* parse <url> */
	url = next;
	next = strchr(next, ' ');
	if (!next || next == url)
		goto error;
	urllen = next - url;

	/* skip " " */
	++next;

	/* parse "RTSP/" */
	if (strncasecmp(next, "RTSP/", 5))
		goto error;
	next += 5;

	/* parse "%u" */
	prev = next;
	shl_atoi_u(prev, 10, (const char**)&next, &major);
	if (next == prev || *next != '.')
		goto error;

	/* skip "." */
	++next;

	/* parse "%u" */
	prev = next;
	shl_atoi_u(prev, 10, (const char**)&next, &minor);
	if (next == prev || *next)
		goto error;

	cmd = strndup(cmd, cmdlen);
	url = strndup(url, urllen);
	if (!cmd || !url) {
		free(cmd);
		return llog_ENOMEM(dec);
	}

	dec->msg.type = WFD_RTSP_MSG_REQUEST;
	dec->msg.id.line = line;
	dec->msg.id.length = len;
	dec->msg.id.request.method = cmd;
	dec->msg.id.request.type = wfd_rtsp_method_from_name(cmd);
	dec->msg.id.request.uri = url;
	dec->msg.id.request.major = major;
	dec->msg.id.request.minor = minor;

	return 0;

error:
	/* Invalid request line.. Set type to UNKNOWN and let the caller deal
	 * with it. We will not try to send any error to avoid triggering
	 * another error if the remote side doesn't understand proper RTSP (or
	 * if our implementation is buggy). */
	dec->msg.type = WFD_RTSP_MSG_UNKNOWN;
	dec->msg.id.line = line;
	dec->msg.id.length = len;
	return 0;
}

static int decoder_parse_response(struct wfd_rtsp_decoder *dec,
				  char *line,
				  size_t len)
{
	unsigned int major, minor, code;
	char *prev, *next, *str;

	/* Responses look like this:
	 *   RTSP/<major>.<minor> <code> <string..>
	 *   RTSP/%u.%u %u %s
	 * We first parse the RTSP version and code. Everything appended to
	 * this is optional and represents the error string. */

	/* skip "RTSP/", already parsed by parent */
	next = &line[5];

	/* parse "%u" */
	prev = next;
	shl_atoi_u(prev, 10, (const char**)&next, &major);
	if (next == prev || *next != '.')
		goto error;

	/* skip "." */
	++next;

	/* parse "%u" */
	prev = next;
	shl_atoi_u(prev, 10, (const char**)&next, &minor);
	if (next == prev || *next != ' ')
		goto error;

	/* skip " " */
	++next;

	/* parse: %u */
	prev = next;
	shl_atoi_u(prev, 10, (const char**)&next, &code);
	if (next == prev)
		goto error;
	if (*next && *next != ' ')
		goto error;

	/* skip " " */
	if (*next)
		++next;

	/* parse: %s */
	str = strdup(next);
	if (!str)
		return llog_ENOMEM(dec);

	dec->msg.type = WFD_RTSP_MSG_RESPONSE;
	dec->msg.id.line = line;
	dec->msg.id.length = len;
	dec->msg.id.response.major = major;
	dec->msg.id.response.minor = minor;
	dec->msg.id.response.status = code;
	dec->msg.id.response.phrase = str;

	return 0;

error:
	/* Couldn't parse line. Avoid sending an error message as we could
	 * trigger another error and end up in an endless error loop. Instead,
	 * set message type to UNKNOWN and let the caller deal with it. */
	dec->msg.type = WFD_RTSP_MSG_UNKNOWN;
	dec->msg.id.line = line;
	dec->msg.id.length = len;
	return 0;
}

static int decoder_parse_id(struct wfd_rtsp_decoder *dec, char *line, size_t len)
{
	if (!strncasecmp(line, "RTSP/", 5))
		return decoder_parse_response(dec, line, len);
	else
		return decoder_parse_request(dec, line, len);
}

/*
 * RTSP Header Parser
 * This parses RTSP header lines. These follow the ID-line and may contain
 * arbitrary additional information. Note that we parse any kind of message that
 * we cannot identify as UNKNOWN. Thus, the caller can implement arbitrary
 * additional parsers.
 *
 * Furthermore, if a header-line cannot be parsed correctly, even though the
 * header-type is known, we still add it as UNKNOWN. Therefore, if the caller
 * implements extensions to *known* header lines, it still needs to go through
 * all unknown lines. It's not enough to go through the lines of the given type.
 */

static int header_append(struct wfd_rtsp_msg_header *h, char *line, size_t len)
{
	char **tlines;
	size_t *tlengths;
	size_t num;

	num = h->count + 2;

	tlines = realloc(h->lines, num * sizeof(*h->lines));
	if (!tlines)
		return -ENOMEM;
	h->lines = tlines;

	tlengths = realloc(h->lengths, num * sizeof(*h->lengths));
	if (!tlengths)
		return -ENOMEM;
	h->lengths = tlengths;

	h->lines[h->count] = line;
	h->lengths[h->count] = len;
	++h->count;
	h->lines[h->count] = NULL;
	h->lengths[h->count] = 0;

	return 0;
}

static int decoder_add_unknown_line(struct wfd_rtsp_decoder *dec,
				    char *line,
				    size_t len)
{
	struct wfd_rtsp_msg_header *h;
	int r;

	/* Cannot parse header line. Append it at the end of the line-array
	 * of type UNKNOWN. Let the caller deal with it. */

	h = &dec->msg.headers[WFD_RTSP_HEADER_UNKNOWN];
	r = header_append(h, line, len);
	return r < 0 ? llog_ERR(dec, r) : 0;
}

static int decoder_parse_content_length(struct wfd_rtsp_decoder *dec,
					char *line,
					size_t len,
					char *tokens,
					size_t num)
{
	struct wfd_rtsp_msg_header *h;
	int r;
	size_t clen;
	char *next;

	h = &dec->msg.headers[WFD_RTSP_HEADER_CONTENT_LENGTH];

	r = shl_atoi_z(tokens, 10, (const char**)&next, &clen);
	if (r < 0 || *next) {
		/* Screwed content-length line? We cannot recover from that as
		 * the attached entity is of unknown length. Abort.. */
		return -EINVAL;
	}

	r = header_append(h, line, len);
	if (r < 0)
		return llog_ERR(dec, r);

	/* overwrite previous lengths */
	h->content_length = clen;
	dec->remaining_body = clen;

	return 0;
}

static int decoder_parse_cseq(struct wfd_rtsp_decoder *dec,
			      char *line,
			      size_t len,
			      char *tokens,
			      size_t num)
{
	struct wfd_rtsp_msg_header *h;
	int r;
	unsigned long val;
	char *next;

	h = &dec->msg.headers[WFD_RTSP_HEADER_CSEQ];

	r = shl_atoi_ul(tokens, 10, (const char**)&next, &val);
	if (r < 0 || *next) {
		/* Screwed cseq line? Append it as unknown line. */
		return decoder_add_unknown_line(dec, line, len);
	}

	r = header_append(h, line, len);
	if (r < 0)
		return llog_ERR(dec, r);

	/* overwrite previous cseqs */
	h->cseq = val;

	return 0;
}

static int decoder_parse_header(struct wfd_rtsp_decoder *dec,
				char *line,
				size_t len)
{
	unsigned int type;
	char *next, *tokens;
	ssize_t num;
	int r;

	num = wfd_rtsp_tokenize(line, len, &tokens);
	if (num < 0)
		return llog_ENOMEM(dec);
	if (num < 2)
		goto error;

	/* Header lines look like this:
	 *   <name>: <value> */
	next = tokens;

	/* parse <name> */
	type = wfd_rtsp_header_from_name(next);
	next = wfd_rtsp_next_token(next);
	--num;

	/* parse ":" */
	if (strcmp(next, ":"))
		goto error;
	next = wfd_rtsp_next_token(next);
	--num;

	/* dispatch to header specific parser */
	switch (type) {
	case WFD_RTSP_HEADER_CONTENT_LENGTH:
		r = decoder_parse_content_length(dec, line, len, next, num);
		break;
	case WFD_RTSP_HEADER_CSEQ:
		r = decoder_parse_cseq(dec, line, len, next, num);
		break;
	default:
		/* no parser for given type available; append to list */
		r = header_append(&dec->msg.headers[type], line, len);
		if (r < 0)
			llog_vERR(dec, r);
		break;
	}

	free(tokens);
	return r;

error:
	free(tokens);
	return decoder_add_unknown_line(dec, line, len);
}

/*
 * Generic Header-line and ID-line parser
 * This parser is invoked on each successfully read header/id-line. It sanitizes
 * the input and then calls the correct header or ID parser, depending on
 * current state.
 */

static size_t sanitize_header_line(struct wfd_rtsp_decoder *dec,
				   char *line, size_t len)
{
	char *src, *dst, c, prev, last_c;
	size_t i;
	bool quoted, escaped;

	src = line;
	dst = line;
	last_c = 0;
	quoted = 0;
	escaped = 0;

	for (i = 0; i < len; ++i) {
		c = *src++;
		prev = last_c;
		last_c = c;

		if (quoted) {
			if (prev == '\\' && !escaped) {
				escaped = 1;
				/* turn escaped binary zero into "\0" */
				if (c == '\0')
					c = '0';
			} else {
				escaped = 0;
				if (c == '"') {
					quoted = 0;
				} else if (c == '\0') {
					/* skip binary 0 */
					continue;
				}
			}
		} else {
			/* ignore any binary 0 */
			if (c == '\0')
				continue;

			/* turn new-lines/tabs into white-space */
			if (c == '\r' || c == '\n' || c == '\t') {
				c = ' ';
				last_c = c;
			}

			/* trim whitespace */
			if (c == ' ' && prev == ' ')
				continue;

			if (c == '"') {
				quoted = 1;
				escaped = 0;
			}
		}

		*dst++ = c;
	}

	/* terminate string with binary zero */
	*dst = 0;

	/* remove trailing whitespace */
	while (dst > line && *(dst - 1) == ' ')
		*--dst = 0;

	/* the decoder already trims leading-whitespace */

	return dst - line;
}

static int decoder_finish_header_line(struct wfd_rtsp_decoder *dec)
{
	char *line;
	size_t l;
	int r;

	line = malloc(dec->buflen + 1);
	if (!line)
		return llog_ENOMEM(dec);

	shl_ring_copy(&dec->buf, line, dec->buflen);
	line[dec->buflen] = 0;
	l = sanitize_header_line(dec, line, dec->buflen);

	if (!dec->msg.id.line)
		r = decoder_parse_id(dec, line, l);
	else
		r = decoder_parse_header(dec, line, l);

	if (r < 0)
		free(line);

	return r;
}

/*
 * State Machine
 * The decoder state-machine is quite simple. We take an input buffer of
 * arbitrary length from the user and feed it byte by byte into the state
 * machine.
 *
 * Parsing RTSP messages is rather troublesome due to the ASCII-nature. It's
 * easy to parse as is, but has lots of corner-cases which we want to be
 * compatible to maybe broken implementations. Thus, we need this
 * state-machine.
 *
 * All we do here is split the endless input stream into header-lines. The
 * header-lines are not handled by the state-machine itself but passed on. If a
 * message contains an entity payload, we parse the body. Otherwise, we submit
 * the message and continue parsing the next one.
 */

_shl_public_
int wfd_rtsp_decoder_new(wfd_rtsp_decoder_event_t event_fn,
			 void *data,
			 wfd_rtsp_log_t log_fn,
			 void *log_data,
			 struct wfd_rtsp_decoder **out)
{
	struct wfd_rtsp_decoder *dec;

	if (!event_fn || !out)
		return llog_dEINVAL(log_fn, data);

	dec = calloc(1, sizeof(*dec));
	if (!dec)
		return llog_dENOMEM(log_fn, data);

	dec->event_fn = event_fn;
	dec->data = data;
	dec->llog = log_fn;
	dec->llog_data = log_data;

	*out = dec;
	return 0;
}

_shl_public_
void wfd_rtsp_decoder_free(struct wfd_rtsp_decoder *dec)
{
	if (!dec)
		return;

	msg_clear(&dec->msg);
	shl_ring_clear(&dec->buf);
	free(dec);
}

_shl_public_
void wfd_rtsp_decoder_reset(struct wfd_rtsp_decoder *dec)
{
	if (!dec)
		return;

	msg_clear(&dec->msg);
	shl_ring_flush(&dec->buf);

	dec->buflen = 0;
	dec->last_chr = 0;
	dec->state = 0;
	dec->remaining_body = 0;

	dec->data_channel = 0;
	dec->data_size = 0;

	dec->quoted = false;
	dec->dead = false;
}

_shl_public_
void wfd_rtsp_decoder_set_data(struct wfd_rtsp_decoder *dec, void *data)
{
	if (!dec)
		return;

	dec->data = data;
}

_shl_public_
void *wfd_rtsp_decoder_get_data(struct wfd_rtsp_decoder *dec)
{
	if (!dec)
		return NULL;

	return dec->data;
}

static int decoder_feed_char_new(struct wfd_rtsp_decoder *dec, char ch)
{
	switch (ch) {
	case '\r':
	case '\n':
	case '\t':
	case ' ':
		/* If no msg has been started, yet, we ignore LWS for
		 * compatibility reasons. Note that they're actually not
		 * allowed, but should be ignored by implementations. */
		++dec->buflen;
		break;
	case '$':
		/* Interleaved data. Followed by 1 byte channel-id and 2-byte
		 * data-length. */
		dec->state = STATE_DATA_HEAD;
		dec->data_channel = 0;
		dec->data_size = 0;

		/* clear any previous whitespace and leading '$' */
		shl_ring_pull(&dec->buf, dec->buflen + 1);
		dec->buflen = 0;
		break;
	default:
		/* Clear any pending data in the ring-buffer and then just
		 * push the char into the buffer. Any char except LWS is fine
		 * here. */
		dec->state = STATE_HEADER;
		dec->remaining_body = 0;

		shl_ring_pull(&dec->buf, dec->buflen);
		dec->buflen = 1;
		break;
	}

	return 0;
}

static int decoder_feed_char_header(struct wfd_rtsp_decoder *dec, char ch)
{
	int r;

	switch (ch) {
	case '\r':
		if (dec->last_chr == '\r' || dec->last_chr == '\n') {
			/* \r\r means empty new-line. We actually allow \r\r\n,
			 * too. \n\r means empty new-line, too, but might also
			 * be finished off as \n\r\n so go to STATE_HEADER_NL
			 * to optionally complete the new-line.
			 * However, if the body is empty, we need to finish the
			 * msg early as there might be no \n coming.. */
			dec->state = STATE_HEADER_NL;

			/* First finish the last header line if any. Don't
			 * include the current \r as it is already part of the
			 * empty following line. */
			r = decoder_finish_header_line(dec);
			if (r < 0)
				return r;

			/* discard buffer *and* whitespace */
			shl_ring_pull(&dec->buf, dec->buflen + 1);
			dec->buflen = 0;

			/* No remaining body. Finish message! */
			if (!dec->remaining_body) {
				r = decoder_submit(dec);
				if (r < 0)
					return r;
			}
		} else {
			/* '\r' following any character just means newline
			 * (optionally followed by \n). We don't do anything as
			 * it might be a continuation line. */
			++dec->buflen;
		}
		break;
	case '\n':
		if (dec->last_chr == '\n') {
			/* We got \n\n, which means we need to finish the
			 * current header-line. If there's no remaining body,
			 * we immediately finish the message and go to
			 * STATE_NEW. Otherwise, we go to STATE_BODY
			 * straight. */

			/* don't include second \n in header-line */
			r = decoder_finish_header_line(dec);
			if (r < 0)
				return r;

			/* discard buffer *and* whitespace */
			shl_ring_pull(&dec->buf, dec->buflen + 1);
			dec->buflen = 0;

			if (dec->remaining_body) {
				dec->state = STATE_BODY;
			} else {
				dec->state = STATE_NEW;
				r = decoder_submit(dec);
				if (r < 0)
					return r;
			}
		} else if (dec->last_chr == '\r') {
			/* We got an \r\n. We cannot finish the header line as
			 * it might be a continuation line. Next character
			 * decides what to do. Don't do anything here.
			 * \r\n\r cannot happen here as it is handled by
			 * STATE_HEADER_NL. */
			++dec->buflen;
		} else {
			/* Same as above, we cannot finish the line as it
			 * might be a continuation line. Do nothing. */
			++dec->buflen;
		}
		break;
	case '\t':
	case ' ':
		/* Whitespace. Simply push into buffer and don't do anything.
		 * In case of a continuation line, nothing has to be done,
		 * either. */
		++dec->buflen;
		break;
	default:
		if (dec->last_chr == '\r' || dec->last_chr == '\n') {
			/* Last line is complete and this is no whitespace,
			 * thus it's not a continuation line.
			 * Finish the line. */

			/* don't include new char in line */
			r = decoder_finish_header_line(dec);
			if (r < 0)
				return r;
			shl_ring_pull(&dec->buf, dec->buflen);
			dec->buflen = 0;
		}

		/* consume character and handle special chars */
		++dec->buflen;
		if (ch == '"') {
			/* go to STATE_HEADER_QUOTE */
			dec->state = STATE_HEADER_QUOTE;
			dec->quoted = false;
		}

		break;
	}

	return 0;
}

static int decoder_feed_char_header_quote(struct wfd_rtsp_decoder *dec, char ch)
{
	if (dec->last_chr == '\\' && !dec->quoted) {
		/* This character is quoted, so copy it unparsed. To handle
		 * double-backslash, we set the "quoted" bit. */
		++dec->buflen;
		dec->quoted = true;
	} else {
		dec->quoted = false;

		/* consume character and handle special chars */
		++dec->buflen;
		if (ch == '"')
			dec->state = STATE_HEADER;
	}

	return 0;
}

static int decoder_feed_char_body(struct wfd_rtsp_decoder *dec, char ch)
{
	char *line;
	int r;

	/* If remaining_body was already 0, the message had no body. Note that
	 * messages without body are finished early, so no need to call
	 * decoder_submit() here. Simply forward @ch to STATE_NEW.
	 * @rlen is usually 0. We don't care and forward it, too. */
	if (!dec->remaining_body) {
		dec->state = STATE_NEW;
		return decoder_feed_char_new(dec, ch);
	}

	/* *any* character is allowed as body */
	++dec->buflen;

	if (!--dec->remaining_body) {
		/* full body received, copy it and go to STATE_NEW */

		line = malloc(dec->buflen + 1);
		if (!line)
			return llog_ENOMEM(dec);

		shl_ring_copy(&dec->buf, line, dec->buflen);
		line[dec->buflen] = 0;

		dec->msg.entity.value = line;
		dec->msg.entity.size = dec->buflen;
		r = decoder_submit(dec);

		dec->state = STATE_NEW;
		shl_ring_pull(&dec->buf, dec->buflen);
		dec->buflen = 0;

		if (r < 0)
			return r;
	}

	return 0;
}

static int decoder_feed_char_header_nl(struct wfd_rtsp_decoder *dec, char ch)
{
	/* STATE_HEADER_NL means we received an empty line ending with \r. The
	 * standard requires a following \n but advises implementations to
	 * accept \r on itself, too.
	 * What we do is to parse a \n as end-of-header and any character as
	 * end-of-header plus start-of-body. Note that we discard anything in
	 * the ring-buffer that has already been parsed (which normally can
	 * nothing, but lets be safe). */

	if (ch == '\n') {
		/* discard transition chars plus new \n */
		shl_ring_pull(&dec->buf, dec->buflen + 1);
		dec->buflen = 0;

		dec->state = STATE_BODY;
		if (!dec->remaining_body)
			dec->state = STATE_NEW;

		return 0;
	} else {
		/* discard any transition chars and push @ch into body */
		shl_ring_pull(&dec->buf, dec->buflen);
		dec->buflen = 0;

		dec->state = STATE_BODY;
		return decoder_feed_char_body(dec, ch);
	}
}

static int decoder_feed_char_data_head(struct wfd_rtsp_decoder *dec, char ch)
{
	uint8_t buf[3];

	/* Read 1 byte channel-id and 2 byte body length. */

	if (++dec->buflen >= 3) {
		shl_ring_copy(&dec->buf, buf, 3);
		shl_ring_pull(&dec->buf, dec->buflen);
		dec->buflen = 0;

		dec->data_channel = buf[0];
		dec->data_size = (((uint16_t)buf[1]) << 8) | (uint16_t)buf[2];
		dec->state = STATE_DATA_BODY;
	}

	return 0;
}

static int decoder_feed_char_data_body(struct wfd_rtsp_decoder *dec, char ch)
{
	uint8_t *buf;
	int r;

	/* Read @dec->data_size bytes of raw data. */

	if (++dec->buflen >= dec->data_size) {
		buf = malloc(dec->data_size + 1);
		if (!buf)
			return llog_ENOMEM(dec);

		/* Not really needed, but in case it's actually a text-payload
		 * make sure it's 0-terminated to work around client bugs. */
		buf[dec->data_size] = 0;

		shl_ring_copy(&dec->buf, buf, dec->data_size);

		r = decoder_submit_data(dec, buf);
		free(buf);

		dec->state = STATE_NEW;
		shl_ring_pull(&dec->buf, dec->buflen);
		dec->buflen = 0;

		if (r < 0)
			return r;
	}

	return 0;
}

static int decoder_feed_char(struct wfd_rtsp_decoder *dec, char ch)
{
	int r = 0;

	switch (dec->state) {
	case STATE_NEW:
		r = decoder_feed_char_new(dec, ch);
		break;
	case STATE_HEADER:
		r = decoder_feed_char_header(dec, ch);
		break;
	case STATE_HEADER_QUOTE:
		r = decoder_feed_char_header_quote(dec, ch);
		break;
	case STATE_HEADER_NL:
		r = decoder_feed_char_header_nl(dec, ch);
		break;
	case STATE_BODY:
		r = decoder_feed_char_body(dec, ch);
		break;
	case STATE_DATA_HEAD:
		r = decoder_feed_char_data_head(dec, ch);
		break;
	case STATE_DATA_BODY:
		r = decoder_feed_char_data_body(dec, ch);
		break;
	}

	return r;
}

_shl_public_
int wfd_rtsp_decoder_feed(struct wfd_rtsp_decoder *dec,
		      const void *buf,
		      size_t len)
{
	char ch;
	size_t i;
	int r;

	if (!dec)
		return -EINVAL;
	if (dec->dead)
		return llog_EINVAL(dec);
	if (!len)
		return 0;
	if (!buf)
		return llog_EINVAL(dec);

	/* We keep dec->buflen as cache for the current parsed-buffer size. We
	 * need to push the whole input-buffer into our parser-buffer and go
	 * through it one-by-one. The parser increments dec->buflen for each of
	 * these and once we're done, we verify our state is consistent. */

	dec->buflen = shl_ring_get_size(&dec->buf);
	r = shl_ring_push(&dec->buf, buf, len);
	if (r < 0) {
		llog_vERR(dec, r);
		goto error;
	}

	for (i = 0; i < len; ++i) {
		ch = ((const char*)buf)[i];
		r = decoder_feed_char(dec, ch);
		if (r < 0)
			goto error;

		dec->last_chr = ch;
	}

	/* check for internal parser inconsistencies; should not happen! */
	if (dec->buflen != shl_ring_get_size(&dec->buf)) {
		llog_error(dec, "internal RTSP parser error");
		r = -EFAULT;
		goto error;
	}

	return 0;

error:
	dec->dead = true;
	return r;
}
