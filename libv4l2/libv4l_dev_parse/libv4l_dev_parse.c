#include <stdio.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <libv4l-plugin.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>

#define LIBV4L2_MAX_CTRL (128)
#define LIBV4L2_MAX_FMT (128)


int main(int argc, char *argv[])
{
    int fd = -1;
	struct v4l2_format fmt;
	struct v4l2_capability cap;
	struct v4l2_fmtdesc fmtdsc;
	struct v4l2_streamparm streamparm;
	struct v4l2_queryctrl qctrl;
	int i, err = 0;
	char *dev_path = NULL;

	if(argc < 2)
	{
		printf("usage:libv4l_dev_parse [dev_path] \n");
		return -1;

	}

	dev_path = argv[1];
	fd = v4l2_open(dev_path, O_RDWR);
	
	if (fd < 0) {
		printf("Cannot open device fd: %d\n", fd);
		return -1;
	}

	err = v4l2_ioctl(fd, VIDIOC_QUERYCAP, &cap);

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

	/*frame format test*/
	memset(&fmtdsc, 0, sizeof(struct v4l2_fmtdesc));
	
	for(i = 0; i < LIBV4L2_MAX_FMT; i++)
	{
		fmtdsc.index = i;
		fmtdsc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		err = v4l2_ioctl(fd, VIDIOC_ENUM_FMT, &fmtdsc);
		if(0 == err)
		{
			printf("-----print enum v4l2 format-----------\n");
			printf("[%s]: fmtdsc.index: %d \n", dev_path, fmtdsc.index);
			printf("[%s]: fmtdsc.type: 0x%x \n", dev_path, fmtdsc.type);
			printf("[%s]: fmtdsc.flags: 0x%x \n", dev_path, fmtdsc.flags);
			printf("[%s]: fmtdsc.description: %s \n", dev_path, fmtdsc.description);
			printf("[%s]: fmtdsc.pixelformat: 0x%x \n", dev_path, fmtdsc.pixelformat);
			printf("\n");

		}
		else
		{
			if(i <= 0)
			{
				printf("VIDIOC_ENUM_FMT failed err:%d\n", err);
			}
			break;
		}
	
		
	}

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	fmt.fmt.pix.width = 1280;
	fmt.fmt.pix.height = 720;

	err = v4l2_ioctl(fd, VIDIOC_S_FMT, &fmt);
	if(0 == err)
	{
		printf("capture is %lx %d x %d\n", (unsigned long) fmt.fmt.pix.pixelformat, 
				fmt.fmt.pix.width, fmt.fmt.pix.height);

	}
	else
	{	
		printf("VIDIOC_S_FMT failed: %d\n", err);
		return -1;
	}
	
	memset(&fmt, 0, sizeof(struct v4l2_format));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	err = v4l2_ioctl(fd, VIDIOC_G_FMT, &fmt);
	if(0 == err)
	{
		printf("-------print get v4l2 format-----------\n");
		printf("[%s]: v4l2_fmt.type: %d \n", dev_path, fmt.type);
		printf("[%s]: v4l2_fmt.fmt.pix.width: %d \n", dev_path, fmt.fmt.pix.width);
		printf("[%s]: v4l2_fmt.fmt.pix.height: %d \n", dev_path, fmt.fmt.pix.height);
		printf("[%s]: v4l2_fmt.fmt.pix.pixelformat: 0x%x \n", dev_path, fmt.fmt.pix.pixelformat);
		printf("[%s]: v4l2_fmt.fmt.pix.field: %d \n", dev_path, fmt.fmt.pix.field);
		printf("[%s]: v4l2_fmt.fmt.pix.bytesperline: %d \n", dev_path, fmt.fmt.pix.bytesperline);
		printf("[%s]: v4l2_fmt.fmt.pix.sizeimage: %d \n", dev_path, fmt.fmt.pix.sizeimage);
		printf("[%s]: v4l2_fmt.fmt.pix.colorspace: %d \n", dev_path, fmt.fmt.pix.colorspace);
		printf("\n");
	}
	else
    {
    	printf("VIDIOC_G_FMT fail err: %d\n", err);
	}

	memset(&streamparm, 0, sizeof(struct v4l2_streamparm));
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    err = v4l2_ioctl(fd, VIDIOC_G_PARM, &streamparm);
	if(0 == err)
	{
		printf("[%s]:streamparm.parm.capture.capability 0x%x \n", 
				dev_path, streamparm.parm.capture.capability);
		printf("[%s]:streamparm.parm.capture.capturemode 0x%x \n", 
				dev_path, streamparm.parm.capture.capturemode);
		printf("[%s]:streamparm.parm.capture.timeperframe.denominator: %d \n",
				dev_path, streamparm.parm.capture.timeperframe.denominator);
		printf("[%s]:streamparm.parm.capture.timeperframe.numerator: %d \n",
				dev_path, streamparm.parm.capture.timeperframe.numerator);
	}
	else
    {
		printf("VIDIOC_G_PARM failed err: %d\n", err);       
    }

	/* v4l2_queryctrl test */
	memset(&qctrl, 0, sizeof(struct v4l2_queryctrl));
	for(i = 0; i < LIBV4L2_MAX_FMT; i++)
	{
		qctrl.id = V4L2_CID_BASE + i;
		err = v4l2_ioctl(fd, VIDIOC_QUERYCTRL, &qctrl);
		
		if(0 == err)
		{
			printf("\n");
			printf("[%s]: qctrl.id: 0x%x \n", dev_path, qctrl.id);
			printf("[%s]: qctrl.type: %d \n", dev_path, qctrl.type);
			printf("[%s]: qctrl.name: %s \n", dev_path, qctrl.name);
			printf("[%s]: qctrl.minimum: %d \n", dev_path, qctrl.minimum);
			printf("[%s]: qctrl.maximum: %d \n", dev_path, qctrl.maximum);
			printf("[%s]: qctrl.step: %d \n", dev_path, qctrl.step);
			printf("[%s]: qctrl.default_value: %d \n", dev_path, qctrl.default_value);
			printf("[%s]: qctrl.flags: 0x%x \n", dev_path, qctrl.flags);
			printf("\n");
		}
	}
	v4l2_close(fd);
    printf("finish libv4l_dev_parse test OK\n");
 
    return 0;
}

