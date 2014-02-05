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

static void parse(struct wfd_wpa_event *ev, const char *event)
{
	int r;

	wfd_wpa_event_init(ev);
	r = wfd_wpa_event_parse(ev, event);
	ck_assert_msg(!r, "cannot parse event %s", event);
	ck_assert(ev->priority < WFD_WPA_EVENT_P_COUNT);
}

static const char *event_list[] = {
	[WFD_WPA_EVENT_UNKNOWN]				= "",
	[WFD_WPA_EVENT_AP_STA_CONNECTED]		= "AP-STA-CONNECTED 00:00:00:00:00:00",
	[WFD_WPA_EVENT_AP_STA_DISCONNECTED]		= "AP-STA-DISCONNECTED 00:00:00:00:00:00",
	[WFD_WPA_EVENT_P2P_DEVICE_FOUND]		= "P2P-DEVICE-FOUND 00:00:00:00:00:00 name=some-name",
	[WFD_WPA_EVENT_P2P_FIND_STOPPED]		= "P2P-FIND-STOPPED",
	[WFD_WPA_EVENT_P2P_GO_NEG_REQUEST]		= "P2P-GO-NEG-REQUEST",
	[WFD_WPA_EVENT_P2P_GO_NEG_SUCCESS]		= "P2P-GO-NEG-SUCCESS role=GO peer_dev=00:00:00:00:00:00",
	[WFD_WPA_EVENT_P2P_GO_NEG_FAILURE]		= "P2P-GO-NEG-FAILURE",
	[WFD_WPA_EVENT_P2P_GROUP_FORMATION_SUCCESS]	= "P2P-GROUP-FORMATION-SUCCESS",
	[WFD_WPA_EVENT_P2P_GROUP_FORMATION_FAILURE]	= "P2P-GROUP-FORMATION-FAILURE",
	[WFD_WPA_EVENT_P2P_GROUP_STARTED]		= "P2P-GROUP-STARTED p2p-wlan0-0 client go_dev_addr=00:00:00:00:00:00",
	[WFD_WPA_EVENT_P2P_GROUP_REMOVED]		= "P2P-GROUP-REMOVED p2p-wlan0-0 GO",
	[WFD_WPA_EVENT_P2P_PROV_DISC_SHOW_PIN]		= "P2P-PROV-DISC-SHOW-PIN 00:00:00:00:00:00 pin",
	[WFD_WPA_EVENT_P2P_PROV_DISC_ENTER_PIN]		= "P2P-PROV-DISC-ENTER-PIN 00:00:00:00:00:00",
	[WFD_WPA_EVENT_P2P_PROV_DISC_PBC_REQ]		= "P2P-PROV-DISC-PBC-REQ 00:00:00:00:00:00",
	[WFD_WPA_EVENT_P2P_PROV_DISC_PBC_RESP]		= "P2P-PROV-DISC-PBC-RESP 00:00:00:00:00:00",
	[WFD_WPA_EVENT_P2P_SERV_DISC_REQ]		= "P2P-SERV-DISC-REQ",
	[WFD_WPA_EVENT_P2P_SERV_DISC_RESP]		= "P2P-SERV-DISC-RESP",
	[WFD_WPA_EVENT_P2P_INVITATION_RECEIVED]		= "P2P-INVITATION-RECEIVED",
	[WFD_WPA_EVENT_P2P_INVITATION_RESULT]		= "P2P-INVITATION-RESULT",
	[WFD_WPA_EVENT_COUNT]				= NULL
};

START_TEST(test_wpa_parser)
{
	struct wfd_wpa_event ev;
	int i;

	parse(&ev, "");
	ck_assert(ev.type == WFD_WPA_EVENT_UNKNOWN);

	parse(&ev, "asdf");
	ck_assert(ev.type == WFD_WPA_EVENT_UNKNOWN);

	for (i = 0; i < WFD_WPA_EVENT_COUNT; ++i) {
		ck_assert_msg(event_list[i] != NULL, "event %d missing", i);
		parse(&ev, event_list[i]);
		ck_assert_msg(ev.type == i, "event %d invalid", i);
	}

	parse(&ev, "<5>AP-STA-CONNECTED 0:0:0:0:0:0");
	ck_assert(ev.priority == WFD_WPA_EVENT_P_MSGDUMP);
	ck_assert(ev.type == WFD_WPA_EVENT_AP_STA_CONNECTED);

	parse(&ev, "<4>AP-STA-CONNECTED 0:0:0:0:0:0");
	ck_assert(ev.priority == WFD_WPA_EVENT_P_ERROR);
	ck_assert(ev.type == WFD_WPA_EVENT_AP_STA_CONNECTED);

	parse(&ev, "<4>AP-STA-CONNECTED2");
	ck_assert(ev.priority == WFD_WPA_EVENT_P_ERROR);
	ck_assert(ev.type == WFD_WPA_EVENT_UNKNOWN);

	parse(&ev, "<4asdf>AP-STA-CONNECTED 0:0:0:0:0:0");
	ck_assert(ev.priority == WFD_WPA_EVENT_P_MSGDUMP);
	ck_assert(ev.type == WFD_WPA_EVENT_AP_STA_CONNECTED);

	parse(&ev, "<4>AP-STA-CONNECTED 0:0:0:0:0:0");
	ck_assert(ev.priority == WFD_WPA_EVENT_P_ERROR);
	ck_assert(ev.type == WFD_WPA_EVENT_AP_STA_CONNECTED);
	ck_assert(ev.raw != NULL);
	ck_assert(!strcmp(ev.raw, "0:0:0:0:0:0"));

	parse(&ev, "<4>AP-STA something else");
	ck_assert(ev.priority == WFD_WPA_EVENT_P_ERROR);
	ck_assert(ev.type == WFD_WPA_EVENT_UNKNOWN);
	ck_assert(!ev.raw);
}
END_TEST

START_TEST(test_wpa_parser_payload)
{
	struct wfd_wpa_event ev;

	parse(&ev, "<4>P2P-DEVICE-FOUND 0:0:0:0:0:0 name=some-name");
	ck_assert(ev.priority == WFD_WPA_EVENT_P_ERROR);
	ck_assert(ev.type == WFD_WPA_EVENT_P2P_DEVICE_FOUND);
	ck_assert(ev.raw != NULL);
	ck_assert(!strcmp(ev.raw, "0:0:0:0:0:0 name=some-name"));
	ck_assert(!strcmp(ev.p.p2p_device_found.peer_mac, "0:0:0:0:0:0"));
	ck_assert(!strcmp(ev.p.p2p_device_found.name, "some-name"));

	parse(&ev, "<4>P2P-DEVICE-FOUND 0:0:0:0:0:0 name=some-'name\\\\\\''");
	ck_assert(ev.priority == WFD_WPA_EVENT_P_ERROR);
	ck_assert(ev.type == WFD_WPA_EVENT_P2P_DEVICE_FOUND);
	ck_assert(ev.raw != NULL);
	ck_assert(!strcmp(ev.p.p2p_device_found.peer_mac, "0:0:0:0:0:0"));
	ck_assert(!strcmp(ev.p.p2p_device_found.name, "some-name\\'"));

	parse(&ev, "<4>P2P-PROV-DISC-SHOW-PIN 0:0:0:0:0:0 1234567890");
	ck_assert(ev.priority == WFD_WPA_EVENT_P_ERROR);
	ck_assert(ev.type == WFD_WPA_EVENT_P2P_PROV_DISC_SHOW_PIN);
	ck_assert(ev.raw != NULL);
	ck_assert(!strcmp(ev.p.p2p_prov_disc_show_pin.peer_mac, "0:0:0:0:0:0"));
	ck_assert(!strcmp(ev.p.p2p_prov_disc_show_pin.pin, "1234567890"));

	parse(&ev, "<4>P2P-GO-NEG-SUCCESS role=GO peer_dev=0:0:0:0:0:0");
	ck_assert(ev.priority == WFD_WPA_EVENT_P_ERROR);
	ck_assert(ev.type == WFD_WPA_EVENT_P2P_GO_NEG_SUCCESS);
	ck_assert(ev.raw != NULL);
	ck_assert(!strcmp(ev.p.p2p_go_neg_success.peer_mac, "0:0:0:0:0:0"));
	ck_assert(ev.p.p2p_go_neg_success.role == WFD_WPA_EVENT_ROLE_GO);

	parse(&ev, "<4>P2P-GROUP-STARTED p2p-wlan0-0 client go_dev_addr=0:0:0:0:0:0");
	ck_assert(ev.priority == WFD_WPA_EVENT_P_ERROR);
	ck_assert(ev.type == WFD_WPA_EVENT_P2P_GROUP_STARTED);
	ck_assert(ev.raw != NULL);
	ck_assert(!strcmp(ev.p.p2p_group_started.go_mac, "0:0:0:0:0:0"));
	ck_assert(!strcmp(ev.p.p2p_group_started.ifname, "p2p-wlan0-0"));
	ck_assert(ev.p.p2p_group_started.role == WFD_WPA_EVENT_ROLE_CLIENT);

	parse(&ev, "<4>P2P-GROUP-REMOVED p2p-wlan0-1 GO");
	ck_assert(ev.priority == WFD_WPA_EVENT_P_ERROR);
	ck_assert(ev.type == WFD_WPA_EVENT_P2P_GROUP_REMOVED);
	ck_assert(ev.raw != NULL);
	ck_assert(!strcmp(ev.p.p2p_group_removed.ifname, "p2p-wlan0-1"));
	ck_assert(ev.p.p2p_group_removed.role == WFD_WPA_EVENT_ROLE_GO);
}
END_TEST

TEST_DEFINE_CASE(parser)
	TEST(test_wpa_parser)
	TEST(test_wpa_parser_payload)
TEST_END_CASE

TEST_DEFINE(
	TEST_SUITE(wpa,
		TEST_CASE(parser),
		TEST_END
	)
)
