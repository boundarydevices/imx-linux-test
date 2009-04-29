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

int mxc_v4l_overlay_setup(struct v4l2_format *fmt)
{
	struct v4l2_streamparm parm;

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = g_camera_framerate;
	parm.parm.capture.capturemode = 0;

	if (ioctl(fd_v4l, VIDIOC_S_PARM, &parm) < 0) {
		printf("VIDIOC_S_PARM failed\n");
		return TFAIL;
	}

	return TPASS;
}

int process_cmdline(int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-help") == 0) {
			printf("MX25 Video4Linux overlay Device Test\n\n" \
				"./mxc_v4l2_overlay -t time\n");
		}
		else if (strcmp(argv[i], "-t") == 0) {
			g_timeout = atoi(argv[++i]);
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct v4l2_format fmt;
	char fb_device[100] = "/dev/fb0";
	int fd_fb = 0;
	struct fb_fix_screeninfo fix;
	unsigned long fb0;

	if (process_cmdline(argc, argv) < 0) {
		return TFAIL;
	}
	if ((fd_v4l = open(v4l_device, O_RDWR, 0)) < 0) {
		printf("Unable to open %s\n", v4l_device);
		return TFAIL;
	}
	printf("open %s succeed!\n", v4l_device);

	if ((fd_fb = open(fb_device, O_RDWR )) < 0) {
		printf("Unable to open frame buffer\n");
		return TFAIL;
	}

	if (ioctl(fd_fb, FBIOGET_FSCREENINFO, &fix) < 0) {
		close(fd_fb);
		printf("ioctl fb FBIOGET_FSCREENINFO failed!\n");
		return TFAIL;
	}

	if (mxc_v4l_overlay_setup(&fmt) < 0) {
		printf("Setup overlay failed.\n");
		return TFAIL;
	}

	fb0 = fix.smem_start;
	if (ioctl(fd_v4l, VIDIOC_OVERLAY, &fb0) < 0) {
		printf("VIDIOC_OVERLAY start failed\n");
		return TFAIL;
	}
	sleep(g_timeout);

	fb0 = 0;
	if (ioctl(fd_v4l, VIDIOC_OVERLAY, &fb0) < 0) {
		printf("VIDIOC_OVERLAY start failed\n");
		return TFAIL;
	}

	close(fd_fb);
	close(fd_v4l);

	return 0;
}
