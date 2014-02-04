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
#include "shl_macro.h"

/*
 * RTSP Tokenizer
 * The RTSP standard is word-based and allows linear-whitespace between any two
 * tokens or special-chars. This tokenizer splits a given line into a list of
 * tokens and returns how many tokens were generated. It also sanitizes the line
 * by removing whitespace, trimming leading/trailing whitespace, removing binary
 * zero characters and decoding escape-sequences.
 *
 * This tokenizer can be used before or after the basic RTSP sanitizer. But note
 * that some RTSP requests or responses contain URIs or other embedded
 * information which should not be tokenized as they don't follow basic RTSP
 * rules (yeah, who came up with that shit..).
 */
_shl_public_
ssize_t wfd_rtsp_tokenize(const char *line, ssize_t len, char **out)
{
	char *t, *dst, c, prev, last_c;
	const char *src;
	size_t num;
	bool quoted, escaped;

	if (!line || !out)
		return -EINVAL;

	/* we need at most twice as much space for all the terminating 0s */
	if (len < 0)
		len = strlen(line);
	t = calloc(2, len + 1);
	if (!t)
		return -ENOMEM;

	num = 0;
	src = line;
	dst = t;
	quoted = false;
	escaped = false;
	prev = 0;
	last_c = 0;

	for ( ; len > 0; --len) {
		c = *src++;
		prev = last_c;
		last_c = 0;

		if (quoted) {
			if (escaped) {
				last_c = c;
				if (c == '\\') {
					*dst++ = '\\';
				} else if (c == '"') {
					*dst++ = '"';
				} else if (c == 'n') {
					*dst++ = '\n';
				} else if (c == 'r') {
					*dst++ = '\r';
				} else if (c == 't') {
					*dst++ = '\t';
				} else if (c == 'a') {
					*dst++ = '\a';
				} else if (c == 'f') {
					*dst++ = '\f';
				} else if (c == 'v') {
					*dst++ = '\v';
				} else if (c == 'b') {
					*dst++ = '\b';
				} else if (c == 'e') {
					*dst++ = 0x1b;	/* ESC */
				} else if (c == 0) {
					/* turn escaped binary 0 into \0 */
					*dst++ = '\\';
					*dst++ = '0';
					last_c = '0';
				} else {
					/* keep unknown escape sequences */
					*dst++ = '\\';
					*dst++ = c;
				}
				escaped = false;
			} else {
				if (c == '"') {
					*dst++ = 0;
					++num;
					quoted = false;
				} else if (c == '\\') {
					escaped = true;
					last_c = prev;
				} else if (c == 0) {
					/* discard */
					last_c = prev;
				} else {
					*dst++ = c;
					last_c = c;
				}
			}
		} else {
			if (c == '"') {
				if (prev) {
					*dst++ = 0;
					++num;
				}
				quoted = true;
			} else if (c == 0) {
				/* discard */
				last_c = prev;
			} else if (c == ' ' ||
				   c == '\t' ||
				   c == '\n' ||
				   c == '\r') {
				if (prev) {
					*dst++ = 0;
					++num;
				}
			} else if (c == '(' ||
				   c == ')' ||
				   c == '[' ||
				   c == ']' ||
				   c == '{' ||
				   c == '}' ||
				   c == '<' ||
				   c == '>' ||
				   c == '@' ||
				   c == ',' ||
				   c == ';' ||
				   c == ':' ||
				   c == '\\' ||
				   c == '/' ||
				   c == '?' ||
				   c == '=') {
				if (prev) {
					*dst++ = 0;
					++num;
				}
				*dst++ = c;
				*dst++ = 0;
				++num;
			} else if (c <= 31 || c == 127) {
				/* ignore CTLs */
				if (prev) {
					*dst++ = 0;
					++num;
				}
			} else {
				*dst++ = c;
				last_c = c;
			}
		}
	}

	if (last_c || quoted) {
		if (escaped)
			*dst++ = '\\';
		*dst++ = 0;
		++num;
	}

	*out = t;
	return num;
}
