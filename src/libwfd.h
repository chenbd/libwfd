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

#ifndef WFD_LIBWFD_H
#define WFD_LIBWFD_H

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WFD__PACKED __attribute__((__packed__))

/**
 * @defgroup wfd_p2p Wifi-Display Definitions for wifi-p2p
 * Definitions from the Wifi-Display specification for Wifi-P2P
 *
 * This section contains definitions and constants from the Wifi-Display
 * specification regarding Wifi-P2P. 
 *
 * @{
 */

/*
 * TODO: WFD 5.2.7 defines service-discovery frames. wpa-supplicant currently
 * does not support custom OUI fields. Fix this and then add support for
 * WFD service discovery.
 */

/*
 * IE elements
 */

#define WFD_IE_ID 0xdd
#define WFD_IE_OUI_1_0 0x506f9a0a
#define WFD_IE_DATA_MAX 251

struct wfd_ie {
	uint8_t element_id;
	uint8_t length;
	uint32_t oui;
	uint8_t data[];
} WFD__PACKED;

/*
 * IE subelements
 */

enum wfd_ie_sub_type {
	WFD_IE_SUB_DEV_INFO			= 0,
	WFD_IE_SUB_ASSOC_BSSID			= 1,
	WFD_IE_SUB_AUDIO_FORMATS		= 2,
	WFD_IE_SUB_VIDEO_FORMATS		= 3,
	WFD_IE_SUB_3D_FORMATS			= 4,
	WFD_IE_SUB_CONTENT_PROTECT		= 5,
	WFD_IE_SUB_COUPLED_SINK			= 6,
	WFD_IE_SUB_EXT_CAP			= 7,
	WFD_IE_SUB_LOCAL_IP			= 8,
	WFD_IE_SUB_SESSION_INFO			= 9,
	WFD_IE_SUB_ALT_MAC			= 10,
	WFD_IE_SUB_NUM
};

struct wfd_ie_sub {
	uint8_t subelement_id;
	uint16_t length;
	uint8_t data[];
} WFD__PACKED;

/*
 * IE subelement device information
 */

/* role */
#define WFD_IE_SUB_DEV_INFO_ROLE_MASK			0x0003
#define WFD_IE_SUB_DEV_INFO_SOURCE			0x0000
#define WFD_IE_SUB_DEV_INFO_PRIMARY_SINK		0x0001
#define WFD_IE_SUB_DEV_INFO_SECONDARY_SINK		0x0002
#define WFD_IE_SUB_DEV_INFO_DUAL_ROLE			0x0003

/* coupled sink as source */
#define WFD_IE_SUB_DEV_INFO_SRC_COUPLED_SINK_MASK	0x0004
#define WFD_IE_SUB_DEV_INFO_SRC_NO_COUPLED_SINK		0x0000
#define WFD_IE_SUB_DEV_INFO_SRC_CAN_COUPLED_SINK	0x0004

/* coupled sink as sink */
#define WFD_IE_SUB_DEV_INFO_SINK_COUPLED_SINK_MASK	0x0008
#define WFD_IE_SUB_DEV_INFO_SINK_NO_COUPLED_SINK	0x0000
#define WFD_IE_SUB_DEV_INFO_SINK_CAN_COUPLED_SINK	0x0008

/* availability for session establishment */
#define WFD_IE_SUB_DEV_INFO_AVAILABLE_MASK		0x0030
#define WFD_IE_SUB_DEV_INFO_NOT_AVAILABLE		0x0000
#define WFD_IE_SUB_DEV_INFO_AVAILABLE			0x0010

/* WFD service discovery */
#define WFD_IE_SUB_DEV_INFO_WSD_MASK			0x0040
#define WFD_IE_SUB_DEV_INFO_NO_WSD			0x0000
#define WFD_IE_SUB_DEV_INFO_CAN_WSD			0x0040

/* preferred connectivity */
#define WFD_IE_SUB_DEV_INFO_PC_MASK			0x0080
#define WFD_IE_SUB_DEV_INFO_PREFER_P2P			0x0000
#define WFD_IE_SUB_DEV_INFO_PREFER_TDLS			0x0080

/* content protection */
#define WFD_IE_SUB_DEV_INFO_CP_MASK			0x0100
#define WFD_IE_SUB_DEV_INFO_NO_CP			0x0000
#define WFD_IE_SUB_DEV_INFO_CAN_CP			0x0100

/* separate time-sync */
#define WFD_IE_SUB_DEV_INFO_TIME_SYNC_MASK		0x0200
#define WFD_IE_SUB_DEV_INFO_NO_TIME_SYNC		0x0000
#define WFD_IE_SUB_DEV_INFO_CAN_TIME_SYNC		0x0200

/* no audio */
#define WFD_IE_SUB_DEV_INFO_NO_AUDIO_MASK		0x0400
#define WFD_IE_SUB_DEV_INFO_CAN_AUDIO			0x0000
#define WFD_IE_SUB_DEV_INFO_NO_AUDIO			0x0400

/* audio only */
#define WFD_IE_SUB_DEV_INFO_AUDIO_ONLY_MASK		0x0800
#define WFD_IE_SUB_DEV_INFO_NO_AUDIO_ONLY		0x0000
#define WFD_IE_SUB_DEV_INFO_AUDIO_ONLY			0x0800

/* persistent TLDS */
#define WFD_IE_SUB_DEV_INFO_PERSIST_TLDS_MASK		0x1000
#define WFD_IE_SUB_DEV_INFO_NO_PERSIST_TLDS		0x0000
#define WFD_IE_SUB_DEV_INFO_PERSIST_TLDS		0x1000

/* persistent TLDS group re-invoke */
#define WFD_IE_SUB_DEV_INFO_TLDS_REINVOKE_MASK		0x2000
#define WFD_IE_SUB_DEV_INFO_NO_TLDS_REINVOKE		0x0000
#define WFD_IE_SUB_DEV_INFO_TLDS_REINVOKE		0x2000

#define WFD_IE_SUB_DEV_INFO_DEFAULT_PORT 7236

struct wfd_ie_sub_dev_info {
	uint16_t dev_info;
	uint16_t ctrl_port;
	uint16_t max_throughput;
} WFD__PACKED;

/*
 * IE subelement associated BSSID
 */

struct wfd_ie_sub_assoc_bssid {
	uint8_t bssid[6];
} WFD__PACKED;

/*
 * IE subelement audio formats
 */

/* lpcm modes; 2C_16_48000 is mandatory */
#define WFD_IE_SUB_AUDIO_FORMATS_LPCM_2C_16_44100	0x00000001
#define WFD_IE_SUB_AUDIO_FORMATS_LPCM_2C_16_48000	0x00000002

/* aac modes */
#define WFD_IE_SUB_AUDIO_FORMATS_AAC_2C_16_48000	0x00000001
#define WFD_IE_SUB_AUDIO_FORMATS_AAC_4C_16_48000	0x00000002
#define WFD_IE_SUB_AUDIO_FORMATS_AAC_6C_16_48000	0x00000004
#define WFD_IE_SUB_AUDIO_FORMATS_AAC_8C_16_48000	0x00000008

/* ac3 modes */
#define WFD_IE_SUB_AUDIO_FORMATS_AC3_2C_16_48000	0x00000001
#define WFD_IE_SUB_AUDIO_FORMATS_AC3_4C_16_48000	0x00000002
#define WFD_IE_SUB_AUDIO_FORMATS_AC3_6C_16_48000	0x00000004

/* audio latency; encoded in multiples of 5ms */
#define WFD_IE_SUB_AUDIO_FORMATS_UNKNOWN_LATENCY	0x00
#define WFD_IE_SUB_AUDIO_FORMATS_LATENCY_FROM_MS(_ms) \
							(((_ms) + 4ULL) / 5ULL)

struct wfd_ie_sub_audio_formats {
	uint32_t lpcm_modes;
	uint8_t lpcm_latency;
	uint32_t aac_modes;
	uint8_t aac_latency;
	uint32_t ac3_modes;
	uint8_t ac3_latency;
} WFD__PACKED;

/*
 * IE subelement video formats
 * Multiple video-subelements are allowed, one for each supported 264 profile.
 */

/* cea modes; required cea modes; 640x480@p60 is always required; if you
 * support higher resolutions at p60 or p50, you also must support 720x480@p60
 * or 720x576@p50 respectively */
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_640_480_P60	0x00000001
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_720_480_P60	0x00000002
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_720_480_I60	0x00000004
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_720_576_P50	0x00000008
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_720_576_I50	0x00000010
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_1280_720_P30	0x00000020
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_1280_720_P60	0x00000040
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_1920_1080_P30	0x00000080
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_1920_1080_P60	0x00000100
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_1920_1080_I60	0x00000200
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_1280_720_P25	0x00000400
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_1280_720_P50	0x00000800
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_1920_1080_P25	0x00001000
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_1920_1080_P50	0x00002000
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_1920_1080_I50	0x00004000
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_1280_720_P24	0x00008000
#define WFD_IE_SUB_VIDEO_FORMATS_CEA_1920_1080_P24	0x00010000

/* vesa modes; if you support higher refresh-rates, you must also support
 * *all* lower rates of the same mode */
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_800_600_P30	0x00000001
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_800_600_P60	0x00000002
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1024_768_P30	0x00000004
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1024_768_P60	0x00000008
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1152_864_P30	0x00000010
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1152_864_P60	0x00000020
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1280_768_P30	0x00000040
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1280_768_P60	0x00000080
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1280_800_P30	0x00000100
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1280_800_P60	0x00000200
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1360_768_P30	0x00000400
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1360_768_P60	0x00000800
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1366_768_P30	0x00001000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1366_768_P60	0x00002000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1280_1024_P30	0x00004000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1280_1024_P60	0x00008000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1400_1050_P30	0x00010000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1400_1050_P60	0x00020000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1440_900_P30	0x00040000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1440_900_P60	0x00080000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1600_900_P30	0x00100000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1600_900_P60	0x00200000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1600_1200_P30	0x00400000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1600_1200_P60	0x00800000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1680_1024_P30	0x01000000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1680_1024_P60	0x02000000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1680_1050_P30	0x04000000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1680_1050_P60	0x08000000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1920_1200_P30	0x10000000
#define WFD_IE_SUB_VIDEO_FORMATS_VESA_1920_1200_P60	0x20000000

/* hh modes (handheld devices) */
#define WFD_IE_SUB_VIDEO_FORMATS_HH_800_480_P30		0x00000000
#define WFD_IE_SUB_VIDEO_FORMATS_HH_800_480_P60		0x00000000
#define WFD_IE_SUB_VIDEO_FORMATS_HH_854_480_P30		0x00000000
#define WFD_IE_SUB_VIDEO_FORMATS_HH_854_480_P60		0x00000000
#define WFD_IE_SUB_VIDEO_FORMATS_HH_864_480_P30		0x00000000
#define WFD_IE_SUB_VIDEO_FORMATS_HH_864_480_P60		0x00000000
#define WFD_IE_SUB_VIDEO_FORMATS_HH_640_360_P30		0x00000000
#define WFD_IE_SUB_VIDEO_FORMATS_HH_640_360_P60		0x00000000
#define WFD_IE_SUB_VIDEO_FORMATS_HH_960_540_P30		0x00000000
#define WFD_IE_SUB_VIDEO_FORMATS_HH_960_540_P60		0x00000000
#define WFD_IE_SUB_VIDEO_FORMATS_HH_848_480_P30		0x00000000
#define WFD_IE_SUB_VIDEO_FORMATS_HH_848_480_P60		0x00000000

/* native mode; table */
#define WFD_IE_SUB_VIDEO_FORMATS_NATIVE_MODE_TABLE_MASK	0x03
#define WFD_IE_SUB_VIDEO_FORMATS_NATIVE_MODE_CEA_TABLE	0x00
#define WFD_IE_SUB_VIDEO_FORMATS_NATIVE_MODE_VESA_TABLE	0x01
#define WFD_IE_SUB_VIDEO_FORMATS_NATIVE_MODE_HH_TABLE	0x02

/* native mode; index */
#define WFD_IE_SUB_VIDEO_FORMATS_NATIVE_MODE_IDX_MASK	0xfc
#define WFD_IE_SUB_VIDEO_FORMATS_NATIVE_MODE_IDX_SHIFT	3

/* h264 profiles; base-profile / high-profile; mostly only one bit allowed */
#define WFD_IE_SUB_VIDEO_FORMATS_PROFILE_CBP		0x01
#define WFD_IE_SUB_VIDEO_FORMATS_PROFILE_CHP		0x02

/* max h264 level; mostly only one bit allowed */
#define WFD_IE_SUB_VIDEO_FORMATS_H264_LEVEL_3_1		0x01
#define WFD_IE_SUB_VIDEO_FORMATS_H264_LEVEL_3_2		0x02
#define WFD_IE_SUB_VIDEO_FORMATS_H264_LEVEL_4_0		0x04
#define WFD_IE_SUB_VIDEO_FORMATS_H264_LEVEL_4_1		0x08
#define WFD_IE_SUB_VIDEO_FORMATS_H264_LEVEL_4_2		0x10

/* display latency; encoded in multiples of 5ms */
#define WFD_IE_SUB_VIDEO_FORMATS_UNKNOWN_LATENCY	0x00
#define WFD_IE_SUB_VIDEO_FORMATS_LATENCY_FROM_MS(_ms) \
							(((_ms) + 4ULL) / 5ULL)

/* smallest slice size expressed in number of macro-blocks or 0x0 */
#define WFD_IE_SUB_VIDEO_FORMATS_NO_SLICES		0x0000

/* if no slices allowed, this can be set on slice_env */
#define WFD_IE_SUB_VIDEO_FORMATS_NO_SLICE_ENC		0x0000

/* max number of slices per picture MINUS 1 (0 not allowed) */
#define WFD_IE_SUB_VIDEO_FORMATS_SLICE_ENC_MAX_MASK	0x03ff
#define WFD_IE_SUB_VIDEO_FORMATS_SLICE_ENC_MAX_SHIFT	0

/* ratio of max-slice-size to be used and slice_min field (0 not allowed) */
#define WFD_IE_SUB_VIDEO_FORMATS_SLICE_ENC_RATIO_MASK	0x0c00
#define WFD_IE_SUB_VIDEO_FORMATS_SLICE_ENC_RATIO_SHIFT	10

/* frame skipping */
#define WFD_IE_SUB_VIDEO_FORMATS_NO_FRAME_SKIP		0x00
#define WFD_IE_SUB_VIDEO_FORMATS_CAN_FRAME_SKIP		0x01

#define WFD_IE_SUB_VIDEO_FORMATS_FRAME_SKIP_MAX_I_MASK	0x0e
#define WFD_IE_SUB_VIDEO_FORMATS_FRAME_SKIP_MAX_I_SHIFT	1
#define WFD_IE_SUB_VIDEO_FORMATS_FRAME_SKIP_MAX_I_ANY	0x00

#define WFD_IE_SUB_VIDEO_FORMATS_FRAME_SKIP_NO_DYN	0x00
#define WFD_IE_SUB_VIDEO_FORMATS_FRAME_SKIP_CAN_DYN	0x10

struct wfd_ie_sub_video_formats {
	uint32_t cea_modes;
	uint32_t vesa_modes;
	uint32_t hh_modes;
	uint8_t native_mode;
	uint8_t h264_profile;
	uint8_t h264_max_level;
	uint8_t latency;
	uint16_t slice_min;
	uint16_t slice_enc;
	uint8_t frame_skip;
} WFD__PACKED;

/*
 * IE subelement 3d formats
 * Multiple 3d-subelements are allowed, one for each supported h264 profile.
 */

/* 3d cpabilities; required modes; 1920x540/540@p24 is always required; if you
 * support higher resolutions at p60 or p50, you also must support
 * 1280x360/360@p60 or 1280x360/360@p50 respectively */
#define WFD_IE_SUB_3D_FORMATS_CAP_1920_X_540_540_P24		0x0000000000000001
#define WFD_IE_SUB_3D_FORMATS_CAP_1280_X_360_360_P60		0x0000000000000002
#define WFD_IE_SUB_3D_FORMATS_CAP_1280_X_360_360_P50		0x0000000000000004
#define WFD_IE_SUB_3D_FORMATS_CAP_1920_X_1080_P24_P24		0x0000000000000008
#define WFD_IE_SUB_3D_FORMATS_CAP_1280_X_720_P60_P60		0x0000000000000010
#define WFD_IE_SUB_3D_FORMATS_CAP_1280_X_720_P30_P30		0x0000000000000020
#define WFD_IE_SUB_3D_FORMATS_CAP_1280_X_720_P50_P50		0x0000000000000040
#define WFD_IE_SUB_3D_FORMATS_CAP_1280_X_720_P25_P25		0x0000000000000080
#define WFD_IE_SUB_3D_FORMATS_CAP_1920_X_1080_45_1080_P24	0x0000000000000100
#define WFD_IE_SUB_3D_FORMATS_CAP_1280_X_720_30_720_P60		0x0000000000000200
#define WFD_IE_SUB_3D_FORMATS_CAP_1280_X_720_30_720_P30		0x0000000000000400
#define WFD_IE_SUB_3D_FORMATS_CAP_1280_X_720_30_720_P50		0x0000000000000800
#define WFD_IE_SUB_3D_FORMATS_CAP_1280_X_720_30_720_P25		0x0000000000001000
#define WFD_IE_SUB_3D_FORMATS_CAP_960_960_X_1080_I60		0x0000000000002000
#define WFD_IE_SUB_3D_FORMATS_CAP_960_960_X_1080_I50		0x0000000000004000
#define WFD_IE_SUB_3D_FORMATS_CAP_640_X_240_240_P60		0x0000000000008000
#define WFD_IE_SUB_3D_FORMATS_CAP_320_320_X_480_P60		0x0000000000010000
#define WFD_IE_SUB_3D_FORMATS_CAP_720_X_240_240_P60		0x0000000000020000
#define WFD_IE_SUB_3D_FORMATS_CAP_360_360_X_480_P60		0x0000000000040000
#define WFD_IE_SUB_3D_FORMATS_CAP_720_X_288_288_P50		0x0000000000080000
#define WFD_IE_SUB_3D_FORMATS_CAP_360_360_X_576_P50		0x0000000000100000
#define WFD_IE_SUB_3D_FORMATS_CAP_1280_X_360_360_P24		0x0000000000200000
#define WFD_IE_SUB_3D_FORMATS_CAP_640_640_X_720_P24		0x0000000000400000
#define WFD_IE_SUB_3D_FORMATS_CAP_1280_X_360_360_P25		0x0000000000800000
#define WFD_IE_SUB_3D_FORMATS_CAP_640_640_X_720_P25		0x0000000001000000
#define WFD_IE_SUB_3D_FORMATS_CAP_1280_X_360_360_P30		0x0000000002000000
#define WFD_IE_SUB_3D_FORMATS_CAP_640_640_X_720_P30		0x0000000004000000
#define WFD_IE_SUB_3D_FORMATS_CAP_1920_X_540_540_P30		0x0000000008000000
#define WFD_IE_SUB_3D_FORMATS_CAP_1920_X_540_540_P50		0x0000000010000000
#define WFD_IE_SUB_3D_FORMATS_CAP_1920_X_540_540_P60		0x0000000020000000
#define WFD_IE_SUB_3D_FORMATS_CAP_640_640_X_720_P50		0x0000000040000000
#define WFD_IE_SUB_3D_FORMATS_CAP_640_640_X_720_P60		0x0000000080000000
#define WFD_IE_SUB_3D_FORMATS_CAP_960_960_X_1080_P24		0x0000000100000000
#define WFD_IE_SUB_3D_FORMATS_CAP_960_960_X_1080_P50		0x0000000200000000
#define WFD_IE_SUB_3D_FORMATS_CAP_960_960_X_1080_P60		0x0000000400000000
#define WFD_IE_SUB_3D_FORMATS_CAP_1920_X_1080_45_1080_P30	0x0000000800000000
#define WFD_IE_SUB_3D_FORMATS_CAP_1920_X_1080_45_1080_I50	0x0000001000000000
#define WFD_IE_SUB_3D_FORMATS_CAP_1920_X_1080_45_1080_I60	0x0000002000000000

struct wfd_ie_sub_3d_formats {
	uint64_t capabilities;
	uint8_t native_mode;	/* same as video_formats.native_mode */
	uint8_t h264_profile;	/* same as video_formats.h264_profile */
	uint8_t h264_max_level;	/* same as video_formats.h264_max_level */
	uint8_t latency;	/* same as video_formats.latency */
	uint16_t slice_min;	/* same as video_formats.slice_min */
	uint16_t slice_enc;	/* same as video_formats.slice_enc */
	uint8_t frame_skip;	/* same as video_formats.frame_skip */
} WFD__PACKED;

/*
 * IE subelement content protection
 */

/* HDCP 2.0 */
#define WFD_IE_SUB_CONTENT_PROTECT_HDCP_2_0_MASK	0x01
#define WFD_IE_SUB_CONTENT_PROTECT_NO_HDCP_2_0		0x00
#define WFD_IE_SUB_CONTENT_PROTECT_CAN_HDCP_2_0		0x01

/* HDCP 2.1; if set, you must also set HDCP 2.0 */
#define WFD_IE_SUB_CONTENT_PROTECT_HDCP_2_1_MASK	0x02
#define WFD_IE_SUB_CONTENT_PROTECT_NO_HDCP_2_1		0x00
#define WFD_IE_SUB_CONTENT_PROTECT_CAN_HDCP_2_1		0x02

struct wfd_ie_sub_content_protect {
	uint8_t flags;
} WFD__PACKED;

/*
 * IE subelement coupled sink information
 */

/* status */
#define WFD_IE_SUB_COUPLED_SINK_STATUS_MASK		0x03
#define WFD_IE_SUB_COUPLED_SINK_NOT_COUPLED		0x00
#define WFD_IE_SUB_COUPLED_SINK_COUPLED			0x01
#define WFD_IE_SUB_COUPLED_SINK_COUPLE_TEARDOWN		0x02

struct wfd_ie_sub_coupled_sink {
	uint8_t status;
	uint8_t mac[6];
} WFD__PACKED;

/*
 * IE subelement extended capabilities
 */

/* UIBC */
#define WFD_IE_SUB_EXT_CAP_UIBC_MASK			0x01
#define WFD_IE_SUB_EXT_CAP_NO_UIBC			0x00
#define WFD_IE_SUB_EXT_CAP_CAN_UIBC			0x01

/* I2C */
#define WFD_IE_SUB_EXT_CAP_I2C_MASK			0x02
#define WFD_IE_SUB_EXT_CAP_NO_I2C			0x00
#define WFD_IE_SUB_EXT_CAP_CAN_I2C			0x02

/* Preferred Mode */
#define WFD_IE_SUB_EXT_CAP_PREFER_MODE_MASK		0x04
#define WFD_IE_SUB_EXT_CAP_NO_PREFER_MODE		0x00
#define WFD_IE_SUB_EXT_CAP_CAN_PREFER_MODE		0x04

/* Standby */
#define WFD_IE_SUB_EXT_CAP_STANDBY_MASK			0x08
#define WFD_IE_SUB_EXT_CAP_NO_STANDBY			0x00
#define WFD_IE_SUB_EXT_CAP_CAN_STANDBY			0x08

/* Persistend TDLS */
#define WFD_IE_SUB_EXT_CAP_PERSIST_TDLS_MASK		0x10
#define WFD_IE_SUB_EXT_CAP_NO_PERSIST_TDLS		0x00
#define WFD_IE_SUB_EXT_CAP_CAN_PERSIST_TDLS		0x10

/* Persistend TDLS BSSID */
#define WFD_IE_SUB_EXT_CAP_PERSIST_TDLS_BSSID_MASK	0x20
#define WFD_IE_SUB_EXT_CAP_NO_PERSIST_TDLS_BSSID	0x00
#define WFD_IE_SUB_EXT_CAP_CAN_PERSIST_TDLS_BSSID	0x20

struct wfd_ie_sub_ext_cap {
	uint16_t flags;
} WFD__PACKED;

/*
 * IE subelement local ip
 */

#define WFD_IE_SUB_LOCAL_IP_IPV4			0x01

struct wfd_ie_sub_local_ip {
	uint8_t version;
	uint8_t ip[4];
} WFD__PACKED;

/*
 * IE subelement session information
 */

/* real payload is actually an array of this object for each device */
struct wfd_ie_sub_session_info {
	uint8_t length;			/* fixed: 23 == (sizeof(this) - 1) */
	uint8_t mac[6];
	uint8_t bssid[6];
	uint16_t dev_info;		/* same as dev_info.dev_info */
	uint16_t max_throughput;	/* same as dev_info.max_throughput */
	uint8_t coupled_status;		/* same as coupled_sink.status */
	uint8_t coupled_mac[6];		/* same as coupled_sink.mac */
} WFD__PACKED;

/*
 * IE subelement alternative mac
 */

struct wfd_ie_sub_alt_mac {
	uint8_t mac[6];
} WFD__PACKED;

/** @} */

/**
 * @defgroup wfd_wpa WPA-Supplicant API
 * Helper API to deal with wpa_supplicant for WFD devices
 *
 * On linux wpa_supplicant is the de-facto standard for wifi-handling. It
 * provides a standard-compiant supplicant implementation with a custom API for
 * applications. This API implements helpers to deal with this daemon and get
 * wifi-P2P connections for WFD working.
 *
 * @{
 */

/* wpa ctrl */

struct wfd_wpa_ctrl;

typedef void (*wfd_wpa_ctrl_event_t) (struct wfd_wpa_ctrl *wpa, void *data,
				      void *buf, size_t len);

int wfd_wpa_ctrl_new(wfd_wpa_ctrl_event_t event_fn, void *data,
		     struct wfd_wpa_ctrl **out);
void wfd_wpa_ctrl_ref(struct wfd_wpa_ctrl *wpa);
void wfd_wpa_ctrl_unref(struct wfd_wpa_ctrl *wpa);

void wfd_wpa_ctrl_set_data(struct wfd_wpa_ctrl *wpa, void *data);
void *wfd_wpa_ctrl_get_data(struct wfd_wpa_ctrl *wpa);

int wfd_wpa_ctrl_open(struct wfd_wpa_ctrl *wpa, const char *ctrl_path);
void wfd_wpa_ctrl_close(struct wfd_wpa_ctrl *wpa);
bool wfd_wpa_ctrl_is_open(struct wfd_wpa_ctrl *wpa);

int wfd_wpa_ctrl_get_fd(struct wfd_wpa_ctrl *wpa);
void wfd_wpa_ctrl_set_sigmask(struct wfd_wpa_ctrl *wpa, const sigset_t *mask);
int wfd_wpa_ctrl_dispatch(struct wfd_wpa_ctrl *wpa, int timeout);

int wfd_wpa_ctrl_request(struct wfd_wpa_ctrl *wpa, const void *cmd,
			 size_t cmd_len, void *reply, size_t *reply_len,
			 int timeout);
int wfd_wpa_ctrl_request_ok(struct wfd_wpa_ctrl *wpa, const void *cmd,
			    size_t cmd_len, int timeout);

/* wpa parser */

enum wfd_wpa_event_type {
	WFD_WPA_EVENT_UNKNOWN,
	WFD_WPA_EVENT_CTRL_EVENT_SCAN_STARTED,
	WFD_WPA_EVENT_CTRL_EVENT_TERMINATING,
	WFD_WPA_EVENT_AP_STA_CONNECTED,
	WFD_WPA_EVENT_AP_STA_DISCONNECTED,
	WFD_WPA_EVENT_P2P_DEVICE_FOUND,
	WFD_WPA_EVENT_P2P_DEVICE_LOST,
	WFD_WPA_EVENT_P2P_FIND_STOPPED,
	WFD_WPA_EVENT_P2P_GO_NEG_REQUEST,
	WFD_WPA_EVENT_P2P_GO_NEG_SUCCESS,
	WFD_WPA_EVENT_P2P_GO_NEG_FAILURE,
	WFD_WPA_EVENT_P2P_GROUP_FORMATION_SUCCESS,
	WFD_WPA_EVENT_P2P_GROUP_FORMATION_FAILURE,
	WFD_WPA_EVENT_P2P_GROUP_STARTED,
	WFD_WPA_EVENT_P2P_GROUP_REMOVED,
	WFD_WPA_EVENT_P2P_PROV_DISC_SHOW_PIN,
	WFD_WPA_EVENT_P2P_PROV_DISC_ENTER_PIN,
	WFD_WPA_EVENT_P2P_PROV_DISC_PBC_REQ,
	WFD_WPA_EVENT_P2P_PROV_DISC_PBC_RESP,
	WFD_WPA_EVENT_P2P_SERV_DISC_REQ,
	WFD_WPA_EVENT_P2P_SERV_DISC_RESP,
	WFD_WPA_EVENT_P2P_INVITATION_RECEIVED,
	WFD_WPA_EVENT_P2P_INVITATION_RESULT,
	WFD_WPA_EVENT_CNT,
};

enum wfd_wpa_event_priority {
	WFD_WPA_EVENT_P_MSGDUMP,
	WFD_WPA_EVENT_P_DEBUG,
	WFD_WPA_EVENT_P_INFO,
	WFD_WPA_EVENT_P_WARNING,
	WFD_WPA_EVENT_P_ERROR,
	WFD_WPA_EVENT_P_CNT
};

enum wfd_wpa_event_role {
	WFD_WPA_EVENT_ROLE_GO,
	WFD_WPA_EVENT_ROLE_CLIENT,
	WFD_WPA_EVENT_ROLE_CNT,
};

#define WFD_WPA_EVENT_MAC_STRLEN 18

struct wfd_wpa_event {
	unsigned int type;
	unsigned int priority;
	char *raw;

	union wfd_wpa_event_payload {
		struct wfd_wpa_event_ap_sta_connected {
			char mac[WFD_WPA_EVENT_MAC_STRLEN];
		} ap_sta_connected;

		struct wfd_wpa_event_ap_sta_disconnected {
			char mac[WFD_WPA_EVENT_MAC_STRLEN];
		} ap_sta_disconnected;

		struct wfd_wpa_event_p2p_device_found {
			char peer_mac[WFD_WPA_EVENT_MAC_STRLEN];
			char *name;
		} p2p_device_found;

		struct wfd_wpa_event_p2p_device_lost {
			char peer_mac[WFD_WPA_EVENT_MAC_STRLEN];
		} p2p_device_lost;

		struct wfd_wpa_event_p2p_go_neg_success {
			char peer_mac[WFD_WPA_EVENT_MAC_STRLEN];
			unsigned int role;
		} p2p_go_neg_success;

		struct wfd_wpa_event_p2p_group_started {
			char go_mac[WFD_WPA_EVENT_MAC_STRLEN];
			unsigned int role;
			char *ifname;
		} p2p_group_started;

		struct wfd_wpa_event_p2p_group_removed {
			unsigned int role;
			char *ifname;
		} p2p_group_removed;

		struct wfd_wpa_event_p2p_prov_disc_show_pin {
			char peer_mac[WFD_WPA_EVENT_MAC_STRLEN];
			char *pin;
		} p2p_prov_disc_show_pin;

		struct wfd_wpa_event_p2p_prov_disc_enter_pin {
			char peer_mac[WFD_WPA_EVENT_MAC_STRLEN];
		} p2p_prov_disc_enter_pin;

		struct wfd_wpa_event_p2p_prov_disc_pbc_req {
			char peer_mac[WFD_WPA_EVENT_MAC_STRLEN];
		} p2p_prov_disc_pbc_req;

		struct wfd_wpa_event_p2p_prov_disc_pbc_resp {
			char peer_mac[WFD_WPA_EVENT_MAC_STRLEN];
		} p2p_prov_disc_pbc_resp;
	} p;
};

void wfd_wpa_event_init(struct wfd_wpa_event *ev);
void wfd_wpa_event_reset(struct wfd_wpa_event *ev);
int wfd_wpa_event_parse(struct wfd_wpa_event *ev, const char *event);
const char *wfd_wpa_event_name(unsigned int type);

/** @} */

/**
 * @defgroup wfd_rtsp Wifi-Display API for RTSP
 * API for the Wifi-Display specification regarding RTSP
 *
 * This section contains definitions and constants from the Wifi-Display
 * specification regarding RTSP and provides a basic API to parse and handle
 * WFD-RTSP messages.
 *
 * Note that this parser is neither fast nor optimized for memory-usage. RTSP is
 * not a high-throughput protocol, therefore, this API is made for easy use, not
 * low-latency and high-throughput. This shouldn't hurt at all. If you use RTSP
 * for huge payloads, you're doing it wrong. Note that this does *NOT* apply to
 * RTSP data messages, which allow combining RTP streams with RTSP. These
 * messages are handled properly and in a fast manner by this decoder.
 *
 * This decoder tries to provide a very convenient API that parses basic types
 * but still allows the caller to get access to any unknown lines. This way,
 * standard-conformant parsers can be easily written, while extensions are still
 * allowed.
 *
 * @{
 */

/**
 * wfd_rtsp_tokenize - Tokenize an RTSP line
 * @line: input line
 * @len: length of the line or -1 if zero-terminated
 * @out: storage pointer for tokens
 *
 * This tokenizes a single RTSP line. It splits the given input-line by RTSP
 * tokens and does some very basic line-parsing. A pointer to the tokenized
 * string is stored in @out and must be freed by the caller. The tokenized
 * string is separated by binary-0 and you can use wfd_rtsp_next_token() to jump
 * to the next token. Binary-0 is not allowed in RTSP lines so this is safe.
 *
 * This function returns the number of tokens parsed. On error, a negative errno
 * code is returned and @out is left untouched.
 */
ssize_t wfd_rtsp_tokenize(const char *line, ssize_t len, char **out);

/**
 * wfd_rtsp_next_token - Return next RTSP token
 * @tokens: toknized RTSP line
 *
 * This can only be used in combination with wfd_rtsp_tokenize. It returns the next
 * token from the tokenized line.
 */
static inline char *wfd_rtsp_next_token(char *tokens)
{
	return tokens + strlen(tokens) + 1;
}

/* RTSP messages */

typedef void (*wfd_rtsp_log_t) (void *data,
				const char *file,
				int line,
				const char *func,
				const char *subs,
				unsigned int sev,
				const char *format,
				va_list args);

enum wfd_rtsp_msg_type {
	WFD_RTSP_MSG_UNKNOWN,

	WFD_RTSP_MSG_REQUEST,
	WFD_RTSP_MSG_RESPONSE,

	WFD_RTSP_MSG_CNT
};

enum wfd_rtsp_method_type {
	WFD_RTSP_METHOD_UNKNOWN,

	WFD_RTSP_METHOD_ANNOUNCE,
	WFD_RTSP_METHOD_DESCRIBE,
	WFD_RTSP_METHOD_GET_PARAMETER,
	WFD_RTSP_METHOD_OPTIONS,
	WFD_RTSP_METHOD_PAUSE,
	WFD_RTSP_METHOD_PLAY,
	WFD_RTSP_METHOD_RECORD,
	WFD_RTSP_METHOD_REDIRECT,
	WFD_RTSP_METHOD_SETUP,
	WFD_RTSP_METHOD_SET_PARAMETER,
	WFD_RTSP_METHOD_TEARDOWN,

	WFD_RTSP_METHOD_CNT
};

const char *wfd_rtsp_method_get_name(unsigned int method);
unsigned int wfd_rtsp_method_from_name(const char *method);

enum wfd_rtsp_status_code {
	WFD_RTSP_STATUS_CONTINUE = 100,

	WFD_RTSP_STATUS_OK = 200,
	WFD_RTSP_STATUS_CREATED,

	WFD_RTSP_STATUS_LOW_ON_STORAGE_SPACE = 250,

	WFD_RTSP_STATUS_MULTIPLE_CHOICES = 300,
	WFD_RTSP_STATUS_MOVED_PERMANENTLY,
	WFD_RTSP_STATUS_MOVED_TEMPORARILY,
	WFD_RTSP_STATUS_SEE_OTHER,
	WFD_RTSP_STATUS_NOT_MODIFIED,
	WFD_RTSP_STATUS_USE_PROXY,

	WFD_RTSP_STATUS_BAD_REQUEST = 400,
	WFD_RTSP_STATUS_UNAUTHORIZED,
	WFD_RTSP_STATUS_PAYMENT_REQUIRED,
	WFD_RTSP_STATUS_FORBIDDEN,
	WFD_RTSP_STATUS_NOT_FOUND,
	WFD_RTSP_STATUS_METHOD_NOT_ALLOWED,
	WFD_RTSP_STATUS_NOT_ACCEPTABLE,
	WFD_RTSP_STATUS_PROXY_AUTHENTICATION_REQUIRED,
	WFD_RTSP_STATUS_REQUEST_TIMEOUT,
	WFD_RTSP_STATUS__PLACEHOLDER__1,
	WFD_RTSP_STATUS_GONE,
	WFD_RTSP_STATUS_LENGTH_REQUIRED,
	WFD_RTSP_STATUS_PRECONDITION_FAILED,
	WFD_RTSP_STATUS_REQUEST_ENTITY_TOO_LARGE,
	WFD_RTSP_STATUS_REQUEST_URI_TOO_LARGE,
	WFD_RTSP_STATUS_UNSUPPORTED_MEDIA_TYPE,

	WFD_RTSP_STATUS_PARAMETER_NOT_UNDERSTOOD = 451,
	WFD_RTSP_STATUS_CONFERENCE_NOT_FOUND,
	WFD_RTSP_STATUS_NOT_ENOUGH_BANDWIDTH,
	WFD_RTSP_STATUS_SESSION_NOT_FOUND,
	WFD_RTSP_STATUS_METHOD_NOT_VALID_IN_THIS_STATE,
	WFD_RTSP_STATUS_HEADER_FIELD_NOT_VALID_FOR_RESOURCE,
	WFD_RTSP_STATUS_INVALID_RANGE,
	WFD_RTSP_STATUS_PARAMETER_IS_READ_ONLY,
	WFD_RTSP_STATUS_AGGREGATE_OPERATION_NOT_ALLOWED,
	WFD_RTSP_STATUS_ONLY_AGGREGATE_OPERATION_ALLOWED,
	WFD_RTSP_STATUS_UNSUPPORTED_TRANSPORT,
	WFD_RTSP_STATUS_DESTINATION_UNREACHABLE,

	WFD_RTSP_STATUS_INTERNAL_SERVER_ERROR = 500,
	WFD_RTSP_STATUS_NOT_IMPLEMENTED,
	WFD_RTSP_STATUS_BAD_GATEWAY,
	WFD_RTSP_STATUS_SERVICE_UNAVAILABLE,
	WFD_RTSP_STATUS_GATEWAY_TIMEOUT,
	WFD_RTSP_STATUS_WFD_RTSP_VERSION_NOT_SUPPORTED,

	WFD_RTSP_STATUS_OPTION_NOT_SUPPORTED = 551,

	WFD_RTSP_STATUS_CNT
};

bool wfd_rtsp_status_is_valid(unsigned int status);
unsigned int wfd_rtsp_status_get_base(unsigned int status);
const char *wfd_rtsp_status_get_description(unsigned int status);

enum wfd_rtsp_header_type {
	WFD_RTSP_HEADER_UNKNOWN,

	WFD_RTSP_HEADER_ACCEPT,
	WFD_RTSP_HEADER_ACCEPT_ENCODING,
	WFD_RTSP_HEADER_ACCEPT_LANGUAGE,
	WFD_RTSP_HEADER_ALLOW,
	WFD_RTSP_HEADER_AUTHORIZATION,
	WFD_RTSP_HEADER_BANDWIDTH,
	WFD_RTSP_HEADER_BLOCKSIZE,
	WFD_RTSP_HEADER_CACHE_CONTROL,
	WFD_RTSP_HEADER_CONFERENCE,
	WFD_RTSP_HEADER_CONNECTION,
	WFD_RTSP_HEADER_CONTENT_BASE,
	WFD_RTSP_HEADER_CONTENT_ENCODING,
	WFD_RTSP_HEADER_CONTENT_LANGUAGE,
	WFD_RTSP_HEADER_CONTENT_LENGTH,
	WFD_RTSP_HEADER_CONTENT_LOCATION,
	WFD_RTSP_HEADER_CONTENT_TYPE,
	WFD_RTSP_HEADER_CSEQ,
	WFD_RTSP_HEADER_DATE,
	WFD_RTSP_HEADER_EXPIRES,
	WFD_RTSP_HEADER_FROM,
	WFD_RTSP_HEADER_HOST,
	WFD_RTSP_HEADER_IF_MATCH,
	WFD_RTSP_HEADER_IF_MODIFIED_SINCE,
	WFD_RTSP_HEADER_LAST_MODIFIED,
	WFD_RTSP_HEADER_LOCATION,
	WFD_RTSP_HEADER_PROXY_AUTHENTICATE,
	WFD_RTSP_HEADER_PROXY_REQUIRE,
	WFD_RTSP_HEADER_PUBLIC,
	WFD_RTSP_HEADER_RANGE,
	WFD_RTSP_HEADER_REFERER,
	WFD_RTSP_HEADER_RETRY_AFTER,
	WFD_RTSP_HEADER_REQUIRE,
	WFD_RTSP_HEADER_RTP_INFO,
	WFD_RTSP_HEADER_SCALE,
	WFD_RTSP_HEADER_SPEED,
	WFD_RTSP_HEADER_SERVER,
	WFD_RTSP_HEADER_SESSION,
	WFD_RTSP_HEADER_TIMESTAMP,
	WFD_RTSP_HEADER_TRANSPORT,
	WFD_RTSP_HEADER_UNSUPPORTED,
	WFD_RTSP_HEADER_USER_AGENT,
	WFD_RTSP_HEADER_VARY,
	WFD_RTSP_HEADER_VIA,
	WFD_RTSP_HEADER_WWW_AUTHENTICATE,

	WFD_RTSP_HEADER_CNT
};

const char *wfd_rtsp_header_get_name(unsigned int header);
unsigned int wfd_rtsp_header_from_name(const char *header);
unsigned int wfd_rtsp_header_from_name_n(const char *header, size_t len);

struct wfd_rtsp_msg {
	unsigned int type;

	struct wfd_rtsp_msg_id {
		char *line;
		size_t length;

		union {
			struct {
				char *method;
				unsigned int type;
				char *uri;
				unsigned int major;
				unsigned int minor;
			} request;

			struct {
				unsigned int major;
				unsigned int minor;
				unsigned int status;
				char *phrase;
			} response;
		};
	} id;

	struct wfd_rtsp_msg_header {
		size_t count;
		char **lines;
		size_t *lengths;

		union {
			size_t content_length;
			unsigned long cseq;
		};
	} headers[WFD_RTSP_HEADER_CNT];

	struct wfd_rtsp_msg_entity {
		void *value;
		size_t size;
	} entity;
};

/* rtsp decoder */

struct wfd_rtsp_decoder;

enum wfd_rtsp_decoder_event_type {
	WFD_RTSP_DECODER_MSG,
	WFD_RTSP_DECODER_DATA,
	WFD_RTSP_DECODER_ERROR,
};

struct wfd_rtsp_decoder_event {
	unsigned int type;

	union {
		struct wfd_rtsp_msg *msg;
		struct wfd_rtsp_decoder_data {
			uint8_t channel;
			uint16_t size;
			uint8_t *value;
		} data;
		struct wfd_rtsp_decoder_error {
			void *data;
			size_t length;
		} error;
	};
};

typedef int (*wfd_rtsp_decoder_event_t) (struct wfd_rtsp_decoder *dec,
					 void *data,
					 struct wfd_rtsp_decoder_event *event);

int wfd_rtsp_decoder_new(wfd_rtsp_decoder_event_t event_fn,
			 void *data,
			 wfd_rtsp_log_t log_fn,
			 void *log_data,
			 struct wfd_rtsp_decoder **out);
void wfd_rtsp_decoder_free(struct wfd_rtsp_decoder *dec);
void wfd_rtsp_decoder_reset(struct wfd_rtsp_decoder *dec);

void wfd_rtsp_decoder_set_data(struct wfd_rtsp_decoder *dec, void *data);
void *wfd_rtsp_decoder_get_data(struct wfd_rtsp_decoder *dec);

int wfd_rtsp_decoder_feed(struct wfd_rtsp_decoder *dec,
			  const void *buf,
			  size_t len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* WFD_LIBWFD_H */
