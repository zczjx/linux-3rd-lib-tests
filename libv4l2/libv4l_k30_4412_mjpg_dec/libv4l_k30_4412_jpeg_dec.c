/*******************************************************************************
* Copyright (C), 2000-2019,  Electronic Technology Co., Ltd.
*                
* @filename: libv4l_k30_4412_jpeg_dec.c 
*                
* @author: Clarence.Zhou <zhou_chenz@163.com> 
*                
* @version:
*                
* @date: 2019-1-1    
*                
* @brief:          
*                  
*                  
* @details:        
*                 
*    
*    
* @comment           
*******************************************************************************/

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <poll.h>
#include <fcntl.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <libv4l-plugin.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <gmodule.h>
/* ffmpeg header*/
#include <libswscale/swscale.h>  
#include <libavutil/pixfmt.h> 
#include <libavutil/imgutils.h>
#include "videodev2_exynos_media.h"
#include "jpeg_hal.h"
#include "bsp_fb.h"
#include "bsp_v4l2_cap.h"

#define LIBV4L2_BUF_NR (4)
#define LIBV4L2_MAX_FMT (16)
#define SWSCALE_PLANES_NR (4)

typedef struct libv4l2_param {
	__u32 xres;
	__u32 yres;
	__u32 pixelformat;
	__u32 fps;
} libv4l2_param;

typedef struct libv4l2_cap_buf {
	int bytes;
	char *addr;
} libv4l2_cap_buf;

int disp_fd = 0;
static struct bsp_fb_var_attr fb_var_attr;
static struct bsp_fb_fix_attr fb_fix_attr;
static struct rgb_frame disp_frame = {
	.addr = NULL,
};

static int convert_to_disp_frame_fmt(struct rgb_frame *dst, 
	struct jpeg_buf *src);

int main(int argc, char **argv)
{
	struct v4l2_format fmt;
	struct v4l2_capability cap;
	struct v4l2_fmtdesc fmtdsc;
	struct v4l2_buffer vbuf_param;
	struct v4l2_requestbuffers req_bufs;
	struct jpeg_buf in_buf;
	struct jpeg_buf out_buf;
	struct libv4l2_param v4l2_param;
	struct libv4l2_cap_buf v4l2_buf[LIBV4L2_BUF_NR];
	struct pollfd fd_set[1];
	int buf_mp_flag = 0;
	int video_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int xres = 640;
	int yres = 480;
	int i, err, vfd, pts = 0;
	int fps = 0;
	char pixformat[32];
	char *dev_path = JPEG_DEC_NODE;
	char *ret;
	char *src_path;
	struct stat image_stat;
	char *image_addr;
	int fd_src;
	int rd_size, total_size= 0;
	struct jpeg_config dec_config;
	
	if(argc < 2)
	{
		printf("usage: libv4l_k30_4412_jpeg_dec [source_path]\n");
		return -1;
	}

	disp_fd = bsp_fb_open_dev("/dev/fb0", &fb_var_attr, &fb_fix_attr);
	fb_var_attr.red_offset = 16;
	fb_var_attr.green_offset = 8;
	fb_var_attr.blue_offset = 0;
	fb_var_attr.transp_offset = 24;
	err = bsp_fb_try_setup(disp_fd, &fb_var_attr);
	
	if(err < 0)
	{
		fprintf(stderr, "could not bsp_fb_try_setup\n");
	}
	
	src_path = argv[1];
	
    // Setup of the dec requests
	vfd = jpeghal_dec_init();

	err = v4l2_ioctl(vfd, VIDIOC_QUERYCAP, &cap);

	if(err < 0)
	{	
		printf("VIDIOC_QUERYCAP failed: %d\n", err);
		return -1;
	}

	printf("[%s]: v4l2_cap.driver: %s \n", dev_path, cap.driver);
	printf("[%s]: v4l2_cap.card: %s \n", dev_path, cap.card);
	printf("[%s]: v4l2_cap.bus_info: %s \n", dev_path, cap.bus_info);
	printf("[%s]: v4l2_cap.version: 0x%x \n", dev_path, cap.version);
	printf("[%s]: v4l2_cap.capabilities: 0x%x \n", dev_path, cap.capabilities);	
	memset(&fmtdsc, 0, sizeof(struct v4l2_fmtdesc));

	for(i = 0; i < LIBV4L2_MAX_FMT; i++)
	{
		fmtdsc.index = i;
		fmtdsc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		err = v4l2_ioctl(vfd, VIDIOC_ENUM_FMT, &fmtdsc);
		
		if (err < 0)
		{
			printf("VIDIOC_ENUM_FMT fail err %d \n", err);
    		break;
		}

		printf("\n");
		printf("------------V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE--------\n");
		printf("[%s]: fmt_dsc.index: %d \n", dev_path, fmtdsc.index);
		printf("[%s]: fmt_dsc.type: 0x%x \n", dev_path, fmtdsc.type);
		printf("[%s]: fmt_dsc.flags: 0x%x \n", dev_path, fmtdsc.flags);
		printf("[%s]: fmt_dsc.description: %s \n", dev_path, fmtdsc.description);
		printf("[%s]: fmt_dsc.pixelformat: 0x%x \n", dev_path, fmtdsc.pixelformat);
		printf("\n");
		
	}

	for(i = 0; i < LIBV4L2_MAX_FMT; i++)
	{
		fmtdsc.index = i;
		fmtdsc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		err = v4l2_ioctl(vfd, VIDIOC_ENUM_FMT, &fmtdsc);
		
		if (err < 0)
		{
			printf("VIDIOC_ENUM_FMT fail err %d \n", err);
    		break;
		}

		printf("\n");
		printf("------------V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE--------\n");
		printf("[%s]: fmt_dsc.index: %d \n", dev_path, fmtdsc.index);
		printf("[%s]: fmt_dsc.type: 0x%x \n", dev_path, fmtdsc.type);
		printf("[%s]: fmt_dsc.flags: 0x%x \n", dev_path, fmtdsc.flags);
		printf("[%s]: fmt_dsc.description: %s \n", dev_path, fmtdsc.description);
		printf("[%s]: fmt_dsc.pixelformat: 0x%x \n", dev_path, fmtdsc.pixelformat);
		printf("\n");
	}
	
	fd_src = bsp_v4l2_open_dev(src_path, &buf_mp_flag);
	v4l2_param.pixelformat = V4L2_PIX_FMT_MJPEG;
	v4l2_param.xres = xres;
	v4l2_param.yres = yres;
	bsp_v4l2_try_setup(fd_src, &v4l2_param, buf_mp_flag);
	err = bsp_v4l2_req_buf(fd_src, v4l2_buf, LIBV4L2_BUF_NR, buf_mp_flag);

	if(err < 0)
	{
		printf("bsp_v4l2_req_buf failed err: %d\n", err);
		return -1;
	}
	
	err = bsp_v4l2_stream_on(fd_src, buf_mp_flag);

	if(err < 0)
	{
		printf("bsp_v4l2_stream_on failed err: %d\n", err);
		return -1;
	}

	dec_config.mode = JPEG_DECODE;
	dec_config.width = xres;
	dec_config.height = yres;
	dec_config.num_planes = 1;
	dec_config.scaled_width = xres;
	dec_config.scaled_height = yres;
	dec_config.sizeJpeg = xres * yres * 2;
	dec_config.pix.dec_fmt.in_fmt = V4L2_PIX_FMT_JPEG_422;
	dec_config.pix.dec_fmt.out_fmt = V4L2_PIX_FMT_YUYV;
	jpeghal_dec_setconfig(vfd, &dec_config);
	jpeghal_dec_getconfig(vfd, &dec_config);
	v4l2_param.xres = dec_config.scaled_width;
	v4l2_param.yres = dec_config.scaled_height;
	v4l2_param.pixelformat = dec_config.pix.dec_fmt.out_fmt;
	printf("v4l2_param.fps: %d\n", v4l2_param.fps);
	printf("v4l2_param.pixelformat: 0x%x\n", v4l2_param.pixelformat);
	printf("v4l2_param.xres: %d\n", v4l2_param.xres);
	printf("v4l2_param.yres: %d\n", v4l2_param.yres);
	in_buf.memory = V4L2_MEMORY_MMAP;
	in_buf.num_planes = 1;
	jpeghal_set_inbuf(vfd, &in_buf);
	out_buf.memory = V4L2_MEMORY_MMAP;
	out_buf.num_planes = 1;
	jpeghal_set_outbuf(vfd, &out_buf);
	// memset(in_buf.start[0], 0xff, dec_config.sizeJpeg);
	printf("image_stat.st_size: %d, in_buf.length[0]: %d\n", 
			image_stat.st_size, in_buf.length[0]);

	
	while(++pts <= 1000)
	{
		err = bsp_v4l2_get_frame(fd_src, &vbuf_param, buf_mp_flag);
		memcpy(in_buf.start[0], v4l2_buf[vbuf_param.index].addr, 
			v4l2_buf[vbuf_param.index].bytes);
		// printf("mjpg head: %x %x %x %x %x\n", v4l2_buf[vbuf_param.index].addr[6],
		// 	v4l2_buf[vbuf_param.index].addr[7], v4l2_buf[vbuf_param.index].addr[8],
		// 	v4l2_buf[vbuf_param.index].addr[9], v4l2_buf[vbuf_param.index].addr[10]);
		in_buf.length[0] = v4l2_buf[vbuf_param.index].bytes;
		err = bsp_v4l2_put_frame_buf(fd_src, &vbuf_param);
		jpeghal_dec_exe(vfd, &in_buf, &out_buf);
		disp_frame.xres = xres;
		disp_frame.yres = yres;
		disp_frame.bits_per_pixel = fb_var_attr.bits_per_pixel;
		disp_frame.bytes_per_line = xres * (disp_frame.bits_per_pixel >> 3);
		disp_frame.bytes = disp_frame.bytes_per_line * yres;
		
		if(NULL == disp_frame.addr)
		{
			disp_frame.addr = malloc(disp_frame.bytes);
		}

		// convert_to_disp_frame_fmt(&disp_frame, &out_buf);
		memcpy(disp_frame.addr, out_buf.start[0], out_buf.length[0]);
		bsp_fb_flush(disp_fd, &fb_var_attr, &fb_fix_attr, &disp_frame);	
	}

	bsp_v4l2_stream_off(fd_src, buf_mp_flag);
	close(fd_src);
	close(vfd);
	printf("jpeg dec finish return 0\n");
    return 0;
}

static int convert_to_disp_frame_fmt(struct rgb_frame *dst, 
	struct jpeg_buf *src)
{
	int ret = 0;
	int i ,err; 
	uint8_t *src_data[SWSCALE_PLANES_NR];  
	int src_linesize[SWSCALE_PLANES_NR];  
	uint8_t *dst_data[SWSCALE_PLANES_NR];  
	int dst_linesize[SWSCALE_PLANES_NR];	
	int rescale_method = SWS_BILINEAR;	
	struct SwsContext *convert_ctx = NULL; 
	enum AVPixelFormat src_av_pixfmt = AV_PIX_FMT_YUYV422; 
	enum AVPixelFormat dst_av_pixfmt = AV_PIX_FMT_BGRA; 
	int src_bpp, dst_bpp = 0; 
	const int src_w = dst->xres;
	const int src_h = dst->yres; 
	const int dst_w = dst->xres;
	const int dst_h = dst->yres;

	if((NULL == dst) || (NULL == src))
		return -1;
	
	src_bpp = av_get_bits_per_pixel(av_pix_fmt_desc_get(src_av_pixfmt)); 
	dst_bpp = av_get_bits_per_pixel(av_pix_fmt_desc_get(dst_av_pixfmt));
	convert_ctx = sws_getContext(src_w, src_h, src_av_pixfmt, 
									dst_w, dst_h, dst_av_pixfmt, 
									SWS_BICUBIC, NULL, NULL, NULL);
	if(NULL == convert_ctx)
	{
		printf("file: %s, line: %d\n", __FILE__, __LINE__);
		printf("[%s] input null pointer!\n" , __FUNCTION__);
		return -1;
	
	}
		
	err = av_image_alloc(src_data, src_linesize, src_w, src_h, src_av_pixfmt, 1); 
	
	if (err < 0) 
	{  
		printf("file: %s, line: %d\n", __FILE__, __LINE__);
		printf("[%s] Could not allocate source image!\n" , __FUNCTION__); 
		ret = -1;
		goto end;
	}  
		
	err = av_image_alloc(dst_data, dst_linesize, dst_w, dst_h, dst_av_pixfmt, 1); 
	
	if (err < 0) 
	{  
		printf("file: %s, line: %d\n", __FILE__, __LINE__);
		printf("[%s] Could not allocate source image!\n" , __FUNCTION__); 
		ret = -1;	
		goto end;
	}

	memcpy(src_data[0], src->start[0], src->length[0]);
	sws_scale(convert_ctx, src_data, src_linesize, 0, src_h, dst_data, dst_linesize);
	memcpy(dst->addr, dst_data[0], (dst_bpp >> 3) * dst_w * dst_h);
	ret = 0;
	
end:
	sws_freeContext(convert_ctx);
	av_freep(&src_data[0]);
	av_freep(&dst_data[0]);
	return ret;

}

