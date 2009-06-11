/*
 * Copyright 2009 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * @file csi_v4l2_overlay.c
 *
 * @brief Mx25 Video For Linux 2 driver test application
 *
 */

#ifdef __cplusplus
extern "C"{
#endif

/*=======================================================================
                                        INCLUDE FILES
=======================================================================*/
/* Standard Include Files */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* Verification Test Environment Include Files */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/types.h>
#include <linux/videodev.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <linux/mxcfb.h>

#define TFAIL -1
#define TPASS 0

char v4l_device[100] = "/dev/video0";
int fd_v4l = 0;
int g_camera_framerate = 0;
int g_timeout = 3600;
static int g_width = 640;
static int g_height = 480;
static int g_bpp = 16;
static unsigned long g_pixelformat = V4L2_PIX_FMT_RGB565;

int csi_v4l_overlay_setup(void)
{
	struct v4l2_streamparm parm;
	struct v4l2_format fmt;

	fd_v4l = open(v4l_device, O_RDWR, 0);
	if (fd_v4l < 0) {
		printf("Unable to open %s\n", v4l_device);
		goto err;
	}
	printf("open /dev/video0 succeed!\n");

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd_v4l, VIDIOC_G_FMT, &fmt) < 0) {
		printf("VIDIOC_G_FMT failed\n");
		close(fd_v4l);
		goto err;
	}

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = g_camera_framerate;
	parm.parm.capture.capturemode = 0;
	if (ioctl(fd_v4l, VIDIOC_S_PARM, &parm) < 0) {
		printf("VIDIOC_S_PARM failed\n");
		close(fd_v4l);
		goto err;
	}

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = g_pixelformat;
	fmt.fmt.pix.width = g_width;
	fmt.fmt.pix.height = g_height;
	fmt.fmt.pix.sizeimage = fmt.fmt.pix.width *
				fmt.fmt.pix.height * g_bpp / 8;
	fmt.fmt.pix.bytesperline = g_width * g_bpp / 8;
	if (ioctl(fd_v4l, VIDIOC_S_FMT, &fmt) < 0) {
		printf("VIDIOC_S_FMT failed\n");
		close(fd_v4l);
		goto err;
	}

	return fd_v4l;
err:
	return TFAIL;
}

int csi_v4l_overlay_test(int fd_v4l, int width, int height)
{
	struct v4l2_framebuffer fb_v4l2;
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	char fb_device[100] = "/dev/fb0";
	int fd_fb = 0;
	int overlay = 1;

	memset(&fb_v4l2, 0, sizeof(fb_v4l2));
	if ((fd_fb = open(fb_device, O_RDWR )) < 0) {
		printf("Unable to open frame buffer\n");
		goto FAIL;
	}

	if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &var) < 0) {
		printf("FBIOGET_VSCREENINFO failed\n");
		goto FAIL;
	}

	/* currently only support 1280*960 mode and 640*480 mode*/
	var.xres_virtual = width;
	var.yres_virtual = height;
	if (ioctl(fd_fb, FBIOPUT_VSCREENINFO, &var) < 0) {
		printf("FBIOPUT_VSCREENINFO failed\n");
		goto FAIL;
	}

	if (ioctl(fd_fb, FBIOGET_FSCREENINFO, &fix) < 0) {
		printf("FBIOGET_FSCREENINFO failed\n");
		goto FAIL;
	}

	fb_v4l2.base = (void *)fix.smem_start;
	if (ioctl(fd_v4l, VIDIOC_S_FBUF, &fb_v4l2) < 0)
	{
		printf("set framebuffer failed\n");
		goto FAIL;
	}

	if (ioctl(fd_v4l, VIDIOC_OVERLAY, &overlay) < 0) {
		printf("VIDIOC_OVERLAY start failed\n");
		goto FAIL;
	}

	sleep(g_timeout);

	overlay = 0;
	if (ioctl(fd_v4l, VIDIOC_OVERLAY, &overlay) < 0) {
		printf("VIDIOC_OVERLAY stop failed\n");
		goto FAIL;
	}

	close(fd_fb);
	close(fd_v4l);
	return TPASS;
FAIL:
	close(fd_fb);
	close(fd_v4l);
	return TFAIL;
}

int main(int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-help") == 0) {
			printf("MX25 Video4Linux overlay Device Test\n"
			"./csi_v4l2_overlay -t time "
					    "-w width -h height -fr rate\n"
			"Note: support 1280*960 and 640*480(default) mode\n"
			"1280*960 mode can't support change rate\n"
			"640*480 mode can support 30fps(default) and 15fps\n");
			return 0;
		} else if (strcmp(argv[i], "-w") == 0) {
			g_width = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-h") == 0) {
			g_height = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-fr") == 0) {
			g_camera_framerate = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-t") == 0) {
			g_timeout = atoi(argv[++i]);
		} else {
			printf("MX25 Video4Linux overlay Device Test\n"
			"./csi_v4l2_overlay -t time "
					    "-w width -h height -fr rate\n"
			"Note: support 1280*960 and 640*480(default) mode\n"
			"1280*960 mode can't support change rate\n"
			"640*480 mode can support 30fps(default) and 15fps\n");
			return 0;
		}
	}

	fd_v4l = csi_v4l_overlay_setup();
	if (fd_v4l < 0) {
		printf("csi_v4l_overlay_setup failed\n");
		return TFAIL;
	}

	i = csi_v4l_overlay_test(fd_v4l, g_width, g_height);
	if (i < 0) {
		printf("csi_v4l_overlay_test failed\n");
		return TFAIL;
	}

	return TPASS;
}

