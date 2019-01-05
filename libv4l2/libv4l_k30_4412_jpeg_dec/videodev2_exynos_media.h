/*
 * Video for Linux Two header file for Exynos
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This header file contains several v4l2 APIs to be proposed to v4l2
 * community and until being accepted, will be used restrictly for Exynos.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_VIDEODEV2_EXYNOS_H
#define __LINUX_VIDEODEV2_EXYNOS_H

/*      Pixel format          FOURCC                     depth  Description  */

/* two planes -- one Y, one Cr + Cb interleaved  */
#define V4L2_PIX_FMT_YUV444_2P v4l2_fourcc('Y', 'U', '2', 'P') /* 24  Y/CbCr */
#define V4L2_PIX_FMT_YVU444_2P v4l2_fourcc('Y', 'V', '2', 'P') /* 24  Y/CrCb */

/* three planes -- one Y, one Cr, one Cb */
#define V4L2_PIX_FMT_YUV444_3P v4l2_fourcc('Y', 'U', '3', 'P') /* 24  Y/Cb/Cr */

/* two non contiguous planes - one Y, one Cr + Cb interleaved  */
/* 21  Y/CrCb 4:2:0  */
#define V4L2_PIX_FMT_NV21M    v4l2_fourcc('N', 'M', '2', '1')
/* 12  Y/CbCr 4:2:0 16x16 macroblocks */
#define V4L2_PIX_FMT_NV12MT_16X16 v4l2_fourcc('V', 'M', '1', '2')

/* three non contiguous planes - Y, Cb, Cr */
/* 12  YVU420 planar */
#define V4L2_PIX_FMT_YVU420M  v4l2_fourcc('Y', 'V', 'U', 'M')

/* compressed formats */
#define V4L2_PIX_FMT_H264_MVC v4l2_fourcc('M', '2', '6', '4') /* H264 MVC */
#define V4L2_PIX_FMT_FIMV     v4l2_fourcc('F', 'I', 'M', 'V') /* FIMV  */
#define V4L2_PIX_FMT_FIMV1    v4l2_fourcc('F', 'I', 'M', '1') /* FIMV1 */
#define V4L2_PIX_FMT_FIMV2    v4l2_fourcc('F', 'I', 'M', '2') /* FIMV2 */
#define V4L2_PIX_FMT_FIMV3    v4l2_fourcc('F', 'I', 'M', '3') /* FIMV3 */
#define V4L2_PIX_FMT_FIMV4    v4l2_fourcc('F', 'I', 'M', '4') /* FIMV4 */
#define V4L2_PIX_FMT_VP8      v4l2_fourcc('V', 'P', '8', '0') /* VP8 */

/* yuv444 of JFIF JPEG */
#define V4L2_PIX_FMT_JPEG_444 v4l2_fourcc('J', 'P', 'G', '4')
/* yuv422 of JFIF JPEG */
#define V4L2_PIX_FMT_JPEG_422 v4l2_fourcc('J', 'P', 'G', '2')
/* yuv420 of JFIF JPEG */
#define V4L2_PIX_FMT_JPEG_420 v4l2_fourcc('J', 'P', 'G', '0')
/* grey of JFIF JPEG */
#define V4L2_PIX_FMT_JPEG_GRAY v4l2_fourcc('J', 'P', 'G', 'G')

/*
 *	C O N T R O L S
 */
/* CID base for Exynos controls (USER_CLASS) */
#define V4L2_CID_EXYNOS_BASE		(V4L2_CTRL_CLASS_USER | 0x2000)

/* for rgb alpha function */
#define V4L2_CID_GLOBAL_ALPHA		(V4L2_CID_EXYNOS_BASE + 1)

/* cacheable configuration */
#define V4L2_CID_CACHEABLE		(V4L2_CID_EXYNOS_BASE + 10)

/* jpeg captured size */
#define V4L2_CID_CAM_JPEG_MEMSIZE	(V4L2_CID_EXYNOS_BASE + 20)
#define V4L2_CID_CAM_JPEG_ENCODEDSIZE	(V4L2_CID_EXYNOS_BASE + 21)

#define V4L2_CID_SET_SHAREABLE		(V4L2_CID_EXYNOS_BASE + 40)

/* TV configuration */
#define V4L2_CID_TV_LAYER_BLEND_ENABLE	(V4L2_CID_EXYNOS_BASE + 50)
#define V4L2_CID_TV_LAYER_BLEND_ALPHA	(V4L2_CID_EXYNOS_BASE + 51)
#define V4L2_CID_TV_PIXEL_BLEND_ENABLE	(V4L2_CID_EXYNOS_BASE + 52)
#define V4L2_CID_TV_CHROMA_ENABLE	(V4L2_CID_EXYNOS_BASE + 53)
#define V4L2_CID_TV_CHROMA_VALUE	(V4L2_CID_EXYNOS_BASE + 54)
#define V4L2_CID_TV_HPD_STATUS		(V4L2_CID_EXYNOS_BASE + 55)
#define V4L2_CID_TV_LAYER_PRIO		(V4L2_CID_EXYNOS_BASE + 56)
#define V4L2_CID_TV_SET_DVI_MODE	(V4L2_CID_EXYNOS_BASE + 57)

/* for color space conversion equation selection */
#define V4L2_CID_CSC_EQ_MODE		(V4L2_CID_EXYNOS_BASE + 100)
#define V4L2_CID_CSC_EQ			(V4L2_CID_EXYNOS_BASE + 101)
#define V4L2_CID_CSC_RANGE		(V4L2_CID_EXYNOS_BASE + 102)

/* for DRM playback scenario */
#define V4L2_CID_USE_SYSMMU		(V4L2_CID_EXYNOS_BASE + 200)
#define V4L2_CID_M2M_CTX_NUM		(V4L2_CID_EXYNOS_BASE + 201)

/* CID base for MFC controls (MPEG_CLASS) */
#define V4L2_CID_MPEG_MFC_BASE		(V4L2_CTRL_CLASS_MPEG | 0x2000)

#define V4L2_CID_MPEG_VIDEO_H264_SEI_FP_AVAIL		\
					(V4L2_CID_MPEG_MFC_BASE + 1)
#define V4L2_CID_MPEG_VIDEO_H264_SEI_FP_ARRGMENT_ID	\
					(V4L2_CID_MPEG_MFC_BASE + 2)
#define V4L2_CID_MPEG_VIDEO_H264_SEI_FP_INFO		\
					(V4L2_CID_MPEG_MFC_BASE + 3)
#define V4L2_CID_MPEG_VIDEO_H264_SEI_FP_GRID_POS	\
					(V4L2_CID_MPEG_MFC_BASE + 4)

#define V4L2_CID_MPEG_MFC51_VIDEO_PACKED_PB		\
					(V4L2_CID_MPEG_MFC_BASE + 5)
#define V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG		\
					(V4L2_CID_MPEG_MFC_BASE + 6)
#define V4L2_CID_MPEG_MFC51_VIDEO_CRC_ENABLE		\
					(V4L2_CID_MPEG_MFC_BASE + 7)
#define V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_LUMA		\
					(V4L2_CID_MPEG_MFC_BASE + 8)
#define V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_CHROMA	\
					(V4L2_CID_MPEG_MFC_BASE + 9)
#define V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_LUMA_BOT	\
					(V4L2_CID_MPEG_MFC_BASE + 10)
#define V4L2_CID_MPEG_MFC51_VIDEO_CRC_DATA_CHROMA_BOT	\
					(V4L2_CID_MPEG_MFC_BASE + 11)
#define V4L2_CID_MPEG_MFC51_VIDEO_CRC_GENERATED		\
					(V4L2_CID_MPEG_MFC_BASE + 12)
#define V4L2_CID_MPEG_MFC51_VIDEO_CHECK_STATE		\
					(V4L2_CID_MPEG_MFC_BASE + 13)
#define V4L2_CID_MPEG_MFC51_VIDEO_DISPLAY_STATUS	\
					(V4L2_CID_MPEG_MFC_BASE + 14)

#define V4L2_CID_MPEG_MFC51_VIDEO_LUMA_ADDR	\
					(V4L2_CID_MPEG_MFC_BASE + 15)
#define V4L2_CID_MPEG_MFC51_VIDEO_CHROMA_ADDR	\
					(V4L2_CID_MPEG_MFC_BASE + 16)

#define V4L2_CID_MPEG_MFC51_VIDEO_STREAM_SIZE		\
					(V4L2_CID_MPEG_MFC_BASE + 17)
#define V4L2_CID_MPEG_MFC51_VIDEO_FRAME_COUNT		\
					(V4L2_CID_MPEG_MFC_BASE + 18)
#define V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TYPE		\
					(V4L2_CID_MPEG_MFC_BASE + 19)
enum v4l2_mpeg_mfc51_video_frame_type {
	V4L2_MPEG_MFC51_VIDEO_FRAME_TYPE_NOT_CODED	= 0,
	V4L2_MPEG_MFC51_VIDEO_FRAME_TYPE_I_FRAME	= 1,
	V4L2_MPEG_MFC51_VIDEO_FRAME_TYPE_P_FRAME	= 2,
	V4L2_MPEG_MFC51_VIDEO_FRAME_TYPE_B_FRAME	= 3,
	V4L2_MPEG_MFC51_VIDEO_FRAME_TYPE_SKIPPED	= 4,
	V4L2_MPEG_MFC51_VIDEO_FRAME_TYPE_OTHERS		= 5,
};

#define V4L2_CID_MPEG_MFC51_VIDEO_H264_INTERLACE	\
					(V4L2_CID_MPEG_MFC_BASE + 20)
#define V4L2_CID_MPEG_MFC51_VIDEO_H264_RC_FRAME_RATE	\
					(V4L2_CID_MPEG_MFC_BASE + 21)
#define V4L2_CID_MPEG_MFC51_VIDEO_MPEG4_VOP_TIME_RES	\
					(V4L2_CID_MPEG_MFC_BASE + 22)
#define V4L2_CID_MPEG_MFC51_VIDEO_MPEG4_VOP_FRM_DELTA	\
					(V4L2_CID_MPEG_MFC_BASE + 23)
#define V4L2_CID_MPEG_MFC51_VIDEO_H263_RC_FRAME_RATE	\
					(V4L2_CID_MPEG_MFC_BASE + 24)

#define V4L2_CID_MPEG_MFC6X_VIDEO_FRAME_DELTA		\
					(V4L2_CID_MPEG_MFC_BASE + 25)

#define V4L2_CID_MPEG_MFC51_VIDEO_I_PERIOD_CH	V4L2_CID_MPEG_VIDEO_GOP_SIZE
#define V4L2_CID_MPEG_MFC51_VIDEO_FRAME_RATE_CH		\
				V4L2_CID_MPEG_MFC51_VIDEO_H264_RC_FRAME_RATE
#define V4L2_CID_MPEG_MFC51_VIDEO_BIT_RATE_CH	V4L2_CID_MPEG_VIDEO_BITRATE

/* proposed CIDs, based on 3.3-rc3 */
#define V4L2_CID_MPEG_VIDEO_VBV_DELAY		(V4L2_CID_MPEG_MFC_BASE + 26)

#define V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED_S_B \
	V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED_AT_SLICE_BOUNDARY

#define V4L2_CID_MPEG_VIDEO_H264_SEI_FRAME_PACKING		\
					(V4L2_CID_MPEG_MFC_BASE + 27)
#define V4L2_CID_MPEG_VIDEO_H264_SEI_FP_CURRENT_FRAME_0		\
					(V4L2_CID_MPEG_MFC_BASE + 28)
#define V4L2_CID_MPEG_VIDEO_H264_SEI_FP_ARRANGEMENT_TYPE	\
					(V4L2_CID_MPEG_MFC_BASE + 29)
					
#define V4L2_CID_MPEG_VIDEO_H264_FMO		(V4L2_CID_MPEG_MFC_BASE + 30)
#define V4L2_CID_MPEG_VIDEO_H264_FMO_MAP_TYPE	(V4L2_CID_MPEG_MFC_BASE + 31)
#define V4L2_CID_MPEG_VIDEO_H264_FMO_SLICE_GROUP	\
					(V4L2_CID_MPEG_MFC_BASE + 32)
#define V4L2_CID_MPEG_VIDEO_H264_FMO_CHANGE_DIRECTION	\
					(V4L2_CID_MPEG_MFC_BASE + 33)
#define V4L2_CID_MPEG_VIDEO_H264_FMO_CHANGE_RATE	\
					(V4L2_CID_MPEG_MFC_BASE + 34)
#define V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH		\
					(V4L2_CID_MPEG_MFC_BASE + 35)
#define V4L2_CID_MPEG_VIDEO_H264_ASO			\
					(V4L2_CID_MPEG_MFC_BASE + 36)
#define V4L2_CID_MPEG_VIDEO_H264_ASO_SLICE_ORDER	\
					(V4L2_CID_MPEG_MFC_BASE + 37)
#define V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING		\
					(V4L2_CID_MPEG_MFC_BASE + 38)
#define V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_TYPE	\
					(V4L2_CID_MPEG_MFC_BASE + 39)
#define V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER	\
					(V4L2_CID_MPEG_MFC_BASE + 40)
#define V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP	\
					(V4L2_CID_MPEG_MFC_BASE + 41)
#define V4L2_CID_MPEG_VIDEO_H264_MVC_VIEW_ID		\
					(V4L2_CID_MPEG_MFC_BASE + 42)
#endif /* __LINUX_VIDEODEV2_EXYNOS_H */

