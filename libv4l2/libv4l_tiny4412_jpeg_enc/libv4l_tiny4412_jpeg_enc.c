/*******************************************************************************
* Copyright (C), 2000-2019,  Electronic Technology Co., Ltd.
*                
* @filename: libv4l_tiny4412_jpeg_enc.c 
*                
* @author: Clarence.Zhou <zhou_chenz@163.com> 
*                
* @version:
*                
* @date: 2019-1-11    
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
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <gmodule.h>
/* ffmpeg header*/
#include <libswscale/swscale.h>  
#include <libavutil/pixfmt.h> 
#include <libavutil/imgutils.h>
#include "jpeg_k44_hal.h"
#include "bsp_fb.h"
#include "bsp_v4l2_cap.h"

#define LIBV4L2_BUF_NR (4)
#define LIBV4L2_MAX_FMT (16)
#define SWSCALE_PLANES_NR (4)

static int buf_mp_flag = 0;
static int disp_fd = 0;
static pthread_mutex_t snap_lock = PTHREAD_MUTEX_INITIALIZER;
static int snapshot_run = 0;
static struct bsp_v4l2_param v4l2_param;
static struct bsp_v4l2_buf v4l2_buf[LIBV4L2_BUF_NR];
static struct v4l2_buffer vbuf_param;
static int xres = 640;
static int yres = 480;
static struct bsp_fb_var_attr fb_var_attr;
static struct bsp_fb_fix_attr fb_fix_attr;
static struct rgb_frame disp_frame = {
	.addr = NULL,
};

static int convert_to_disp_frame_fmt(struct rgb_frame *dst, 
	struct bsp_v4l2_buf *src);

static void *snapshot_thread(void *arg);


int main(int argc, char **argv)
{
	struct v4l2_format fmt;
	struct v4l2_capability cap;
	struct v4l2_fmtdesc fmtdsc;
	struct v4l2_requestbuffers req_bufs;
	struct pollfd fd_set[1];
	int video_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int i, buf_cnt, err, pts = 0;
	int fps = 0;
	char pixformat[32];
	char *ret;
	char *src_path, *enc_path;
	struct stat image_stat;
	int fd_src;
	int rd_size, total_size= 0;
	pthread_t snap_tid;
	
	if(argc < 3)
	{
		printf("usage: libv4l_tiny4412_jpeg_enc [source_path] [encode_path]\n");
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
	enc_path = argv[2];
	fd_src = bsp_v4l2_open_dev(src_path, &buf_mp_flag);
	v4l2_param.pixelformat = V4L2_PIX_FMT_YUYV;
	v4l2_param.xres = xres;
	v4l2_param.yres = yres;
	bsp_v4l2_try_setup(fd_src, &v4l2_param, (buf_mp_flag ? 
		V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE));
	printf("v4l2_param.fps: %d\n", v4l2_param.fps);
	printf("v4l2_param.pixelformat: 0x%x\n", v4l2_param.pixelformat);
	printf("v4l2_param.xres: %d\n", v4l2_param.xres);
	printf("v4l2_param.yres: %d\n", v4l2_param.yres);
	buf_cnt = bsp_v4l2_req_buf(fd_src, v4l2_buf, LIBV4L2_BUF_NR, (buf_mp_flag ?
			V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE),
			(buf_mp_flag ? 1 : 0));

	if(buf_cnt < 0)
	{
		printf("bsp_v4l2_req_buf failed err: %d\n", err);
		return -1;
	}

	for(i = 0; i < buf_cnt; i++)
	{
		vbuf_param.index = i;
		vbuf_param.type = (buf_mp_flag ? 
			V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE);
		vbuf_param.memory = V4L2_MEMORY_MMAP;
		err = bsp_v4l2_put_frame_buf(fd_src, &vbuf_param);
		if(err < 0)
		{
			printf("bsp_v4l2_put_frame_buf err: %d, line: %d\n", err, __LINE__);
			return -1;
		}
	}

	err = bsp_v4l2_stream_on(fd_src, (buf_mp_flag ? 
			V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE));

	if(err < 0)
	{
		printf("bsp_v4l2_stream_on failed err: %d\n", err);
		return -1;
	}

	snapshot_run = 1;
	err = pthread_create(&snap_tid, NULL, snapshot_thread, enc_path);
	
	while(++pts <= 1000)
	{
		struct pollfd fd_set[1];

		fd_set[0].fd = fd_src;
		fd_set[0].events = POLLIN;
		err = poll(fd_set, 1, -1);
		err = bsp_v4l2_get_frame_buf(fd_src, &vbuf_param, (buf_mp_flag ? 
			V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE),
			(buf_mp_flag ? 1 : 0));
		disp_frame.xres = xres;
		disp_frame.yres = yres;
		disp_frame.bits_per_pixel = fb_var_attr.bits_per_pixel;
		disp_frame.bytes_per_line = xres * (disp_frame.bits_per_pixel >> 3);
		disp_frame.bytes = disp_frame.bytes_per_line * yres;
		
		if(NULL == disp_frame.addr)
		{
			disp_frame.addr = malloc(disp_frame.bytes);
		}

		if(buf_mp_flag)
		{
			for(i = 0; i < 1; i++)
			{
				v4l2_buf[vbuf_param.index].bytes[i] = 
					vbuf_param.m.planes[i].bytesused;
			}
		}
		else
		{
			v4l2_buf[vbuf_param.index].bytes[0] = vbuf_param.bytesused;
		}

		convert_to_disp_frame_fmt(&disp_frame, &v4l2_buf[vbuf_param.index]);
		pthread_mutex_lock(&snap_lock);
		err = bsp_v4l2_put_frame_buf(fd_src, &vbuf_param);
		pthread_mutex_unlock(&snap_lock);
		bsp_fb_flush(disp_fd, &fb_var_attr, &fb_fix_attr, &disp_frame);	
	}
	
	snapshot_run = 0;
	bsp_v4l2_stream_off(fd_src, 
		(buf_mp_flag ? 
		V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE));
	close(fd_src);
	printf("jpeg enc finish return 0\n");
    return 0;
}

static int convert_to_disp_frame_fmt(struct rgb_frame *dst, 
	struct bsp_v4l2_buf *src)
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

	memcpy(src_data[0], src->addr[0], src->bytes[0]);
	sws_scale(convert_ctx, src_data, src_linesize, 0, src_h, dst_data, dst_linesize);
	memcpy(dst->addr, dst_data[0], (dst_bpp >> 3) * dst_w * dst_h);
	ret = 0;
	
end:
	sws_freeContext(convert_ctx);
	av_freep(&src_data[0]);
	av_freep(&dst_data[0]);
	return ret;

}

static void *snapshot_thread(void *arg)
{
	int vfd = -1;
	struct timespec tp;
	struct jpeg_buf in_buf;
	struct jpeg_buf out_buf;
	struct jpeg_config enc_config;
	int fd_img = -1;
	char *image_addr;
	char cmd;
	char img_name[64];
	char *enc_path = (char *) arg;
	
	while(1 == snapshot_run)
	{
		printf("enter 's' to do snapshot: \n");
		scanf("%c", &cmd);
		
		if('s' == cmd)
		{
			/* it is a workaround solution that open/close jpeg encode
			   in each picture to WAR the linux-4.x encode driver stuck
			   the system when continuous qbuf/dqbuf
			*/

			// Setup of the dec requests
			vfd = jpeghal_enc_init(enc_path);
			enc_config.mode = JPEG_ENCODE;
			enc_config.enc_qual = QUALITY_LEVEL_1;
			enc_config.width = xres;
			enc_config.height = yres;
			enc_config.num_planes = 1;
			enc_config.scaled_width = xres;
			enc_config.scaled_height = yres;
			enc_config.sizeJpeg = xres * yres * 2;
			enc_config.pix.enc_fmt.in_fmt = V4L2_PIX_FMT_YUYV;
			enc_config.pix.enc_fmt.out_fmt = V4L2_PIX_FMT_JPEG;
			jpeghal_enc_setconfig(vfd, &enc_config);
			jpeghal_enc_getconfig(vfd, &enc_config);
			in_buf.memory = V4L2_MEMORY_MMAP;
			in_buf.num_planes = 1;
			jpeghal_set_inbuf(vfd, &in_buf);
			out_buf.memory = V4L2_MEMORY_MMAP;
			out_buf.num_planes = 1;
			jpeghal_set_outbuf(vfd, &out_buf);
			clock_gettime(CLOCK_MONOTONIC, &tp);
			sprintf(img_name, "img%x%x.jpg", tp.tv_sec, tp.tv_nsec);
			fd_img = open(img_name, O_CREAT | O_RDWR);
			pthread_mutex_lock(&snap_lock);
			memcpy(in_buf.start[0], v4l2_buf[vbuf_param.index].addr[0], 
				v4l2_buf[vbuf_param.index].bytes[0]);
			pthread_mutex_unlock(&snap_lock);
			jpeghal_enc_exe(vfd, &in_buf, &out_buf);
			// v4l2_ioctl(vfd, VIDIOC_LOG_STATUS, NULL);
			write(fd_img, out_buf.start[0], out_buf.bytesused);
			close(fd_img);
			jpeghal_deinit(vfd, &in_buf, &out_buf);
			
		}	
		else
		{
			continue;
		}
	}

}


