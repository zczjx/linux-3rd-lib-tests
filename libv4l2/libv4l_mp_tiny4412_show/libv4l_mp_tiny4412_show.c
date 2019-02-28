/*******************************************************************************
* Copyright (C), 2000-2018,  Electronic Technology Co., Ltd.
*                
* @filename: libv4l_tiny4412_show.c 
*                
* @author: Clarence.Zhou <zhou_chenz@163.com> 
*                
* @version:
*                
* @date: 2018-11-25    
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

#include <sys/time.h>
#include <stdlib.h>
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
#include "bsp_fb.h"

#define LIBV4L2_BUF_NR (4)
#define LIBV4L2_MAX_FMT (16)
#define SWSCALE_PLANES_NR (4)

typedef struct libv4l2_cap_buf {
	int bytes;
	char *addr;
} libv4l2_cap_buf;

typedef struct libv4l2_param {
	__u32 xres;
	__u32 yres;
	__u32 pixelformat;
	__u32 fps;
} libv4l2_param;

int disp_fd = 0;
static struct bsp_fb_var_attr fb_var_attr;
static struct bsp_fb_fix_attr fb_fix_attr;
static struct rgb_frame disp_frame = {
	.addr = NULL,
};

static void init_v4l2_name_fmt_set(GData **name_to_fmt_set, 
	GTree *fmt_to_name_set);

static gint fmt_val_cmp(gconstpointer	a, gconstpointer b);

static void libv4l2_print_fps(const char *fsp_dsc, long *fps, 
	long *pre_time, long *curr_time);

static int convert_to_disp_frame_fmt(struct rgb_frame *dst, 
	struct libv4l2_cap_buf *src);


int main(int argc, char **argv)
{
	struct timespec tp;
	struct v4l2_format fmt;
	struct v4l2_capability cap;
	struct v4l2_fmtdesc fmtdsc;
	struct v4l2_streamparm streamparm;
	struct v4l2_queryctrl qctrl;
	struct v4l2_buffer vbuf_param;
	struct v4l2_requestbuffers req_bufs;
	struct v4l2_input input;
	struct v4l2_subdev_mbus_code_enum mbus_code;
	struct v4l2_subdev_format subdev_format;
	struct libv4l2_cap_buf v4l2_buf[LIBV4L2_BUF_NR];
	struct libv4l2_param v4l2_param;
	struct v4l2_plane mplane;
	struct pollfd fd_set[1];
	int video_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	GData *name_to_fmt_set = NULL;
	GTree *fmt_to_name_set = NULL;
	int i, xres, yres;
	int err, vfd, pts = 0;
	long pre_time = 0;
	long curr_time = 0;
	int fps = 0;
	char pixformat[32];
	char *dev_path = NULL;
	char *ret;
	char *src_subdev = NULL, *sink_subdev = NULL;
	int fd_src, fd_sink;
	__u32 code;
	

	if(argc < 6)
	{
		printf("usage: libv4l_tiny4412_show ");
		printf("[dev_path] [src_subdev] [sink_subdev] [xres] [yres]\n");
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

	g_datalist_init(&name_to_fmt_set);
	fmt_to_name_set = g_tree_new(fmt_val_cmp);
	dev_path = argv[1];
	src_subdev = argv[2];
	sink_subdev = argv[3];
	xres = atoi(argv[4]);
	yres = atoi(argv[5]);
	init_v4l2_name_fmt_set(&name_to_fmt_set, fmt_to_name_set);
	
    // Setup of the dec requests
	vfd = open(dev_path, O_RDWR);
	
	if (vfd < 0) {
		printf("Cannot open device fd: %d\n", vfd);
		return -1;
	}

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
	
	v4l2_param.fps = 30;
	input.index = 0;
	err = v4l2_ioctl(vfd, VIDIOC_S_INPUT, &input);
	
	if (err < 0) {
        printf("VIDIOC_S_INPUT failed err: %d\n", err);
 
    }
	
	memset(&fmtdsc, 0, sizeof(struct v4l2_fmtdesc));

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
		
		ret = NULL;
		ret = g_tree_lookup(fmt_to_name_set, fmtdsc.pixelformat);
		
		if(NULL != ret)
		{
			printf("\n");
			printf("enter %s to select %s \n", ret, fmtdsc.description);
			printf("\n");
		}
		
	}

renter:
	printf("\n");
	printf("please enter your pixformat choice: \n");
	printf("\n");
	scanf("%s", pixformat);
	printf("your choice: %s\n", pixformat);
	v4l2_param.pixelformat = g_datalist_get_data(&name_to_fmt_set, pixformat);
	printf("v4l2_param.pixelformat: 0x%x\n", v4l2_param.pixelformat);
	
	if(NULL == v4l2_param.pixelformat)
	{
		printf("invalid pixformat: %s\n", pixformat);
		goto renter;
	}

	memset(&fmt, 0, sizeof(struct v4l2_format));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.pixelformat = v4l2_param.pixelformat;
	fmt.fmt.pix_mp.width = xres;
	fmt.fmt.pix_mp.height = yres;
	fmt.fmt.pix_mp.num_planes = 1;
	fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
	err = v4l2_ioctl(vfd, VIDIOC_S_FMT, &fmt);

	if (err) 
	{
		printf("VIDIOC_S_FMT failed: %d\n", err);
		return -1;		
	}
	
	v4l2_param.xres = fmt.fmt.pix.width;
	v4l2_param.yres = fmt.fmt.pix.height;
	v4l2_param.pixelformat = fmt.fmt.pix.pixelformat;
	v4l2_param.fps = 30;
	printf("v4l2_param.fps: %d \n", v4l2_param.fps);
	printf("v4l2_param.pixelformat: 0x%x \n", v4l2_param.pixelformat);
	printf("v4l2_param.xres: %d \n", v4l2_param.xres);
	printf("v4l2_param.yres: %d \n", v4l2_param.yres);

	fd_src = open(src_subdev, O_RDWR);

	if(fd_src < 0)
	{
		return fd_src;
	}

	fd_sink = open(sink_subdev, O_RDWR);

	if(fd_sink < 0)
	{
		return fd_sink;
	}

	memset(&mbus_code, 0, sizeof(struct v4l2_subdev_mbus_code_enum));

	printf("--------enum v4l2 subdev source format-------------\n");
	for(i = 0; i < 32; i++)
	{
		mbus_code.index = i;
		err = v4l2_ioctl(fd_src, VIDIOC_SUBDEV_ENUM_MBUS_CODE, &mbus_code);
	
		if (err < 0)
    	{
			break;
		}

	
		printf("[%s]: mbus_code.pad: %d \n", dev_path, mbus_code.pad);
		printf("[%s]: mbus_code.index: %d \n", dev_path, mbus_code.index);
		printf("[%s]: mbus_code.code: 0x%x \n", dev_path, mbus_code.code);
		printf("\n");
	}
	printf("--------------------------------------\n");
	printf("\n");
	printf("--------enum v4l2 subdev sink format--------------\n");

	for(i = 0; i < 32; i++)
	{
		mbus_code.index = i;
		err = v4l2_ioctl(fd_sink, VIDIOC_SUBDEV_ENUM_MBUS_CODE, &mbus_code);
	
		if (err < 0)
    	{
			break;
		}
		
		printf("[%s]: mbus_code.pad: %d \n", dev_path, mbus_code.pad);
		printf("[%s]: mbus_code.index: %d \n", dev_path, mbus_code.index);
		printf("[%s]: mbus_code.code: 0x%x \n", dev_path, mbus_code.code);
		printf("\n");
	}

	printf("\n");
	printf("please enter your mbus_code choice:  \n");
	scanf("%x", &code);
	printf("\n");
	printf("your input mbus_code is 0x%x\n", code);

	memset(&subdev_format, 0, sizeof(struct v4l2_subdev_format));
	subdev_format.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	subdev_format.pad  = 0;
	subdev_format.format.width = xres;
	subdev_format.format.height = yres;
	subdev_format.format.code = code;

	err = v4l2_ioctl(fd_sink, VIDIOC_SUBDEV_S_FMT, &subdev_format);
	
	if (err < 0)
	{
		printf("fd_sink VIDIOC_SUBDEV_S_FMT failed err: %d\n", err);
		return -1;
	}

	memset(&subdev_format, 0, sizeof(struct v4l2_subdev_format));
	subdev_format.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	subdev_format.pad  = 0;
	subdev_format.format.width = xres;
	subdev_format.format.height = yres;
	subdev_format.format.code = code;
	err = v4l2_ioctl(fd_src, VIDIOC_SUBDEV_S_FMT, &subdev_format);
	
	if (err < 0)
	{
		printf("fd_src VIDIOC_SUBDEV_S_FMT failed err: %d\n", err);
		return -1;
	}

	memset(&subdev_format, 0, sizeof(struct v4l2_subdev_format));
	subdev_format.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	err = v4l2_ioctl(fd_src, VIDIOC_SUBDEV_G_FMT, &subdev_format);

	printf("[%s]: subdev_format.which: %d \n", src_subdev, subdev_format.which);
	printf("[%s]: subdev_format.pad: %d \n", src_subdev, subdev_format.pad);
	printf("[%s]: subdev_format.format.width: %d \n", 
		src_subdev, subdev_format.format.width);
	printf("[%s]: subdev_format.format.height: %d \n", 
		src_subdev, subdev_format.format.height);
	printf("[%s]: subdev_format.format.code: 0x%x \n", 
		src_subdev, subdev_format.format.code);
	printf("[%s]: subdev_format.format.field: %d \n", 
		src_subdev, subdev_format.format.field);
	printf("[%s]: subdev_format.format.colorspace: %d \n", 
		src_subdev, subdev_format.format.colorspace);
	printf("[%s]: subdev_format.format.ycbcr_enc: %d \n", 
		src_subdev, subdev_format.format.ycbcr_enc);
	printf("[%s]: subdev_format.format.quantization: %d \n", 
		src_subdev, subdev_format.format.quantization);

	memset(&subdev_format, 0, sizeof(struct v4l2_subdev_format));
	subdev_format.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	err = v4l2_ioctl(fd_sink, VIDIOC_SUBDEV_G_FMT, &subdev_format);
		
	printf("[%s]: subdev_format.which: %d \n", sink_subdev, subdev_format.which);
	printf("[%s]: subdev_format.pad: %d \n", sink_subdev, subdev_format.pad);
	printf("[%s]: subdev_format.format.width: %d \n", 
		sink_subdev, subdev_format.format.width);
	printf("[%s]: subdev_format.format.height: %d \n", 
		sink_subdev, subdev_format.format.height);
	printf("[%s]: subdev_format.format.code: 0x%x \n", 
		sink_subdev, subdev_format.format.code);
	printf("[%s]: subdev_format.format.field: %d \n", 
		sink_subdev, subdev_format.format.field);
	printf("[%s]: subdev_format.format.colorspace: %d \n", 
		sink_subdev, subdev_format.format.colorspace);
	printf("[%s]: subdev_format.format.ycbcr_enc: %d \n", 
		sink_subdev, subdev_format.format.ycbcr_enc);
	printf("[%s]: subdev_format.format.quantization: %d \n", 
		sink_subdev, subdev_format.format.quantization);
	

	/* init buf */
	memset(&req_bufs, 0, sizeof(struct v4l2_requestbuffers));
	req_bufs.count = LIBV4L2_BUF_NR;
	req_bufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	req_bufs.memory = V4L2_MEMORY_MMAP;
	err = v4l2_ioctl(vfd, VIDIOC_REQBUFS, &req_bufs);
		
	if (err) 
	{
		printf("VIDIOC_REQBUFS fail err: %d\n", err);
		return -1;		  
	}
		
	for(i = 0; i < LIBV4L2_BUF_NR; i++)
	{
		memset(&vbuf_param, 0, sizeof(struct v4l2_buffer));
		vbuf_param.index = i;
		vbuf_param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		vbuf_param.memory = V4L2_MEMORY_MMAP;
		vbuf_param.m.planes = &mplane;
		vbuf_param.length = 1;
		err = v4l2_ioctl(vfd, VIDIOC_QUERYBUF, &vbuf_param);
		
		if (err) 
		{
			printf("VIDIOC_QUERYBUF fail err: %d\n", err);
			return -1;
		}
				
		v4l2_buf[i].bytes = vbuf_param.m.planes->length;
		v4l2_buf[i].addr = mmap(0 , v4l2_buf[i].bytes, 
								PROT_READ, MAP_SHARED, vfd, 
								vbuf_param.m.planes->m.mem_offset);
		err = v4l2_ioctl(vfd, VIDIOC_QBUF, &vbuf_param);
				
		if (err) 
		{
			printf("VIDIOC_QBUF fail err: %d\n", err);
			return -1;
		}
	
	}


	err = v4l2_ioctl(vfd, VIDIOC_STREAMON, &video_type);
	
	if (err < 0) 
    {
		printf("VIDIOC_STREAMON fail err: %d\n", err);
		return -1;
    }

	while(++pts <= 1000)
	{
		fd_set[0].fd     = vfd;
    	fd_set[0].events = POLLIN;
    	err = poll(fd_set, 1, -1);

		memset(&vbuf_param, 0, sizeof(struct v4l2_buffer));
   		vbuf_param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
   		vbuf_param.memory = V4L2_MEMORY_MMAP;
		vbuf_param.m.planes = &mplane;
		vbuf_param.length = 1;
    	err = v4l2_ioctl(vfd, VIDIOC_DQBUF, &vbuf_param);
		
		if (err < 0) 
		{
			printf("VIDIOC_DQBUF fail err: %d\n", err);
		}
			
		// v4l2_ioctl(vfd, VIDIOC_LOG_STATUS, NULL);
	
		libv4l2_print_fps("bsp_v4l2_fps: ", &fps, &pre_time, &curr_time);

		disp_frame.xres = xres;
		disp_frame.yres = yres;
		disp_frame.bits_per_pixel = fb_var_attr.bits_per_pixel;
		disp_frame.bytes_per_line = xres * (disp_frame.bits_per_pixel >> 3);
		disp_frame.bytes = disp_frame.bytes_per_line * yres;
		
		if(NULL == disp_frame.addr)
		{
			disp_frame.addr = malloc(disp_frame.bytes);
		}

		convert_to_disp_frame_fmt(&disp_frame, &v4l2_buf[vbuf_param.index]);
		// memcpy(disp_frame.addr, v4l2_buf[vbuf_param.index].addr, v4l2_buf[vbuf_param.index].bytes);
		bsp_fb_flush(disp_fd, &fb_var_attr, &fb_fix_attr, &disp_frame);
		err = v4l2_ioctl(vfd, VIDIOC_QBUF, &vbuf_param);
	}

	close(fd_src);
	close(fd_sink);
	close(vfd);
    return 0;
}

static void init_v4l2_name_fmt_set(GData **name_to_fmt_set, 
	GTree *fmt_to_name_set)
{
	char pixformat[32];
	
	g_datalist_set_data(name_to_fmt_set, "yuv420", V4L2_PIX_FMT_YVU420);
	g_datalist_set_data(name_to_fmt_set, "yuyv", V4L2_PIX_FMT_YUYV);
	g_datalist_set_data(name_to_fmt_set, "rgb888", V4L2_PIX_FMT_RGB24);
	g_datalist_set_data(name_to_fmt_set, "jpeg", V4L2_PIX_FMT_JPEG);
	g_datalist_set_data(name_to_fmt_set, "h264", V4L2_PIX_FMT_H264);
	g_datalist_set_data(name_to_fmt_set, "mjpeg", V4L2_PIX_FMT_MJPEG);
	g_datalist_set_data(name_to_fmt_set, "yvyu", V4L2_PIX_FMT_YVYU);
	g_datalist_set_data(name_to_fmt_set, "vyuy", V4L2_PIX_FMT_VYUY);
	g_datalist_set_data(name_to_fmt_set, "uyvy", V4L2_PIX_FMT_UYVY);
	g_datalist_set_data(name_to_fmt_set, "nv12", V4L2_PIX_FMT_NV12);
	g_datalist_set_data(name_to_fmt_set, "bgr888", V4L2_PIX_FMT_BGR24);
	g_datalist_set_data(name_to_fmt_set, "yvu420", V4L2_PIX_FMT_YVU420);
	g_datalist_set_data(name_to_fmt_set, "nv21", V4L2_PIX_FMT_NV21);
	g_datalist_set_data(name_to_fmt_set, "bgr32", V4L2_PIX_FMT_BGR32);

	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_YVU420, "yuv420");
	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_YUYV, "yuyv");
	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_RGB24, "rgb888");
	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_JPEG, "jpeg");
	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_H264, "h264");
	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_MJPEG, "mjpeg");
	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_YVYU, "yvyu");
	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_VYUY, "vyuy");
	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_UYVY, "uyvy");
	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_NV12, "nv12");
	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_BGR24, "bgr888");
	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_YVU420, "yvu420");
	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_NV21, "nv21");
	g_tree_insert(fmt_to_name_set, V4L2_PIX_FMT_BGR32, "bgr32");

}

	
static gint fmt_val_cmp(gconstpointer	a, gconstpointer b)
{
	__u32 key1 = (__u32) a;
	__u32 key2 = (__u32) b;
	
	if(key1 == key2)
		return 0;
	
	return (key1 > key2) ? 1 : -1;
}

static void libv4l2_print_fps(const char *fsp_dsc, long *fps, 
	long *pre_time, long *curr_time)
{
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);
	*curr_time = tp.tv_sec;
	(*fps)++;
		
	if(((*curr_time) - (*pre_time)) >= 1)
	{	
		printf("%s fps: %d \n", fsp_dsc, *fps);
		*pre_time = *curr_time;
		*fps = 0;
	}
	
}

static int convert_to_disp_frame_fmt(struct rgb_frame *dst, 
	struct libv4l2_cap_buf *src)
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

	memcpy(src_data[0], src->addr, src->bytes);
	sws_scale(convert_ctx, src_data, src_linesize, 0, src_h, dst_data, dst_linesize);
	memcpy(dst->addr, dst_data[0], (dst_bpp >> 3) * dst_w * dst_h);
	ret = 0;
	
end:
	sws_freeContext(convert_ctx);
	av_freep(&src_data[0]);
	av_freep(&dst_data[0]);
	return ret;

}

