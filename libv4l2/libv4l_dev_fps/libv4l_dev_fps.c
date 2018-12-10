/*******************************************************************************
* Copyright (C), 2000-2018,  Electronic Technology Co., Ltd.
*                
* @filename: libv4l_dev_fps.c 
*                
* @author: Clarence.Zhou <zhou_chenz@163.com> 
*                
* @version:
*                
* @date: 2018-11-18    
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
#include <libv4l-plugin.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <gmodule.h>

#define LIBV4L2_BUF_NR (4)
#define LIBV4L2_MAX_FMT (128)

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


static void init_v4l2_name_fmt_set(GData **name_to_fmt_set, 
	GTree *fmt_to_name_set);

static gint fmt_val_cmp(gconstpointer	a, gconstpointer b);

static void libv4l2_print_fps(const char *fsp_dsc, long *fps, 
	long *pre_time, long *curr_time);


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
	struct libv4l2_cap_buf v4l2_buf[LIBV4L2_BUF_NR];
	struct libv4l2_param v4l2_param;
	struct pollfd fd_set[1];
	int video_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
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

	if(argc < 4)
	{
		printf("usage: libv4l_dev_fps [dev_path] [xres] [yres]\n");
		return -1;

	}

	g_datalist_init(&name_to_fmt_set);
	fmt_to_name_set = g_tree_new(fmt_val_cmp);
	dev_path = argv[1];
	xres = atoi(argv[2]);
	yres = atoi(argv[3]);
	init_v4l2_name_fmt_set(&name_to_fmt_set, fmt_to_name_set);
	
    // Setup of the dec requests
	vfd = v4l2_open(dev_path, O_RDWR);
	
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
	memset(&fmtdsc, 0, sizeof(struct v4l2_fmtdesc));

	for(i = 0; i < LIBV4L2_MAX_FMT; i++)
	{
		fmtdsc.index = i;
		fmtdsc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
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
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = v4l2_param.pixelformat;
	fmt.fmt.pix.width = xres;
	fmt.fmt.pix.height = yres;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	err = v4l2_ioctl(vfd, VIDIOC_S_FMT, &fmt); 

	if (err) 
	{
		printf("VIDIOC_S_FMT failed: %d\n", err);
		return -1;		
	}

	err = v4l2_ioctl(vfd, VIDIOC_G_FMT, &fmt);

	if (err < 0)
	{
		printf("VIDIOC_G_FMT fail err: %d\n", err);
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

	/* init buf */
	memset(&req_bufs, 0, sizeof(struct v4l2_requestbuffers));
	req_bufs.count = LIBV4L2_BUF_NR;
	req_bufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
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
		vbuf_param.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		vbuf_param.memory = V4L2_MEMORY_MMAP;
		err = v4l2_ioctl(vfd, VIDIOC_QUERYBUF, &vbuf_param);
				
		if (err) 
		{
			printf("VIDIOC_QUERYBUF fail err: %d\n", err);
			return -1;
		}

		printf("VIDIOC_QUERYBUF vbuf_param.length: %d\n", vbuf_param.length);
				
		v4l2_buf[i].bytes = vbuf_param.length;
		v4l2_buf[i].addr = v4l2_mmap(0 , v4l2_buf[i].bytes, 
									PROT_READ, MAP_SHARED, vfd, 
									vbuf_param.m.offset);
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
    	err = v4l2_ioctl(vfd, VIDIOC_DQBUF, &vbuf_param);
		
		if (err < 0) 
		{
			printf("VIDIOC_DQBUF fail err: %d\n", err);
		}
			
		err = v4l2_ioctl(vfd, VIDIOC_QBUF, &vbuf_param);
		
		if (err < 0) 
		{
			printf("VIDIOC_QBUF fail err: %d\n", err);
		}
		libv4l2_print_fps("bsp_v4l2_fps: ", &fps, &pre_time, &curr_time);
	}
	
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

