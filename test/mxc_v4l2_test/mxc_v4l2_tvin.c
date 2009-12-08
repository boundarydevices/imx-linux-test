/*
 * Copyright 2007-2009 Freescale Semiconductor, Inc. All rights reserved.
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
 * @file mxc_v4l2_tvin.c
 *
 * @brief Mxc TVIN For Linux 2 driver test application
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
#include <linux/videodev2.h>
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
int g_cap_mode = 0;
v4l2_std_id g_current_std = V4L2_STD_PAL;

/* PAL active frame size is 720 * 576, output is interlaced mode */
#define PAL_WIDTH             720
#define PAL_HEIGHT            288
/* Display area on LCD */
#define DISPLAY_WIDTH         720
#define DISPLAY_HEIGHT        464
#define DISPLAY_TOP           12
#define DISPLAY_LEFT          40

int
mxc_v4l_tvin_test(void)
{
        int overlay = 1;
	v4l2_std_id id;
	struct v4l2_streamparm parm;

        parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        parm.parm.capture.timeperframe.numerator = 1;
        parm.parm.capture.timeperframe.denominator = 0;
        parm.parm.capture.capturemode = 0;
        if (ioctl(fd_v4l, VIDIOC_OVERLAY, &overlay) < 0)
        {
                printf("VIDIOC_OVERLAY start failed\n");
		return TFAIL;
        }

       /* Detect standard monitor, the default video standard is PAL when the test programe is started */

       while (1) {
		if (ioctl(fd_v4l, VIDIOC_G_STD, &id) < 0)
                {
                    printf("overlay test programe: VIDIOC_G_STD failed\n");
                    return TFAIL;
                }
		if (g_current_std == id)
		{
                    sleep(1);
		    continue;
		}
		else
		{
		    printf("overlay test programe: video standard changed ... \n");
		    ioctl(fd_v4l, VIDIOC_S_STD, &id);
		    g_cap_mode = (g_cap_mode + 1) % 2;
		    parm.parm.capture.capturemode = g_cap_mode;
		    ioctl(fd_v4l, VIDIOC_S_PARM, &parm);
		    g_current_std = id;
                    sleep(1);
		    continue;
		 }

       }

    return 0;
}

int
mxc_v4l_tvin_setup(struct v4l2_format *fmt)
{
        struct v4l2_streamparm parm;
        struct v4l2_control ctl;
        struct v4l2_crop crop;
	 int display_lcd = 0;

        if (ioctl(fd_v4l, VIDIOC_S_OUTPUT, &display_lcd) < 0)
        {
                printf("VIDIOC_S_OUTPUT failed\n");
                return TFAIL;
        }

        ctl.id = V4L2_CID_PRIVATE_BASE;
	ctl.value = 0;
        if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl) < 0)
        {
                printf("set control failed\n");
                return TFAIL;
        }

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        parm.parm.capture.timeperframe.numerator = 1;
        parm.parm.capture.timeperframe.denominator = 0;
        parm.parm.capture.capturemode = 0;
	if (ioctl(fd_v4l, VIDIOC_S_PARM, &parm) < 0)
        {
                printf("VIDIOC_S_PARM failed\n");
                return TFAIL;
        }

	parm.parm.capture.timeperframe.numerator = 0;
        parm.parm.capture.timeperframe.denominator = 0;
	if (ioctl(fd_v4l, VIDIOC_G_PARM, &parm) < 0)
        {
                printf("get frame rate failed\n");
                return TFAIL;
        }

	g_cap_mode = parm.parm.capture.capturemode;
        printf("cap_mode is %d\n", parm.parm.capture.capturemode);
        crop.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
        crop.c.left = 0;
        crop.c.top = 0;
        crop.c.width = PAL_WIDTH;
        crop.c.height = PAL_HEIGHT;
        if (ioctl(fd_v4l, VIDIOC_S_CROP, &crop) < 0)
        {
                printf("set cropping failed\n");
                return TFAIL;
        }

        if (ioctl(fd_v4l, VIDIOC_S_FMT, fmt) < 0)
        {
                printf("set format failed\n");
                return TFAIL;
        }

        if (ioctl(fd_v4l, VIDIOC_G_FMT, fmt) < 0)
        {
                printf("get format failed\n");
                return TFAIL;
        }

        return TPASS;
}

int
main(int argc, char **argv)
{
        struct v4l2_format fmt;
        struct v4l2_framebuffer fb_v4l2;
        char fb_device[100] = "/dev/fb0";
        int fd_fb = 0;
        struct fb_fix_screeninfo fix;
        struct fb_var_screeninfo var;
        struct mxcfb_color_key color_key;
        struct mxcfb_gbl_alpha alpha;
        unsigned short * fb0;
        unsigned char * cur_fb8;
        unsigned short * cur_fb16;
        unsigned int * cur_fb32;
        __u32 screen_size;
        int h, w;
	int ret = 0;

        if ((fd_v4l = open(v4l_device, O_RDWR, 0)) < 0) {
                printf("Unable to open %s\n", v4l_device);
                return TFAIL;
        }

        fmt.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
        fmt.fmt.win.w.top =  DISPLAY_TOP ;
        fmt.fmt.win.w.left = DISPLAY_LEFT;
        fmt.fmt.win.w.width = DISPLAY_WIDTH;
        fmt.fmt.win.w.height = DISPLAY_HEIGHT;

        if (mxc_v4l_tvin_setup(&fmt) < 0) {
                printf("Setup tvin failed.\n");
                return TFAIL;
	}

        memset(&fb_v4l2, 0, sizeof(fb_v4l2));

        if ((fd_fb = open(fb_device, O_RDWR )) < 0)	{
                printf("Unable to open frame buffer\n");
                return TFAIL;
        }

        if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &var) < 0) {
                close(fd_fb);
                return TFAIL;
        }
        if (ioctl(fd_fb, FBIOGET_FSCREENINFO, &fix) < 0) {
                close(fd_fb);
                return TFAIL;
        }

        /* Overlay setting */
        alpha.alpha = 0;
	alpha.enable = 1;
	if ( ioctl(fd_fb, MXCFB_SET_GBL_ALPHA, &alpha) < 0) {
                close(fd_fb);
                return TFAIL;
	}

        color_key.color_key = 0x00080808;
        color_key.enable = 1;
        if ( ioctl(fd_fb, MXCFB_SET_CLR_KEY, &color_key) < 0) {
                close(fd_fb);
                return TFAIL;
        }

        screen_size = var.yres * fix.line_length; // 480*800

        /* Map the device to memory*/
        fb0 = (unsigned short *)mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
        if ((int)fb0 == -1)
        {
                printf("\nError: failed to map framebuffer device 0 to memory.\n");
                close(fd_fb);
                return TFAIL;
	 }

        if (var.bits_per_pixel == 16) {
                for (h = DISPLAY_TOP; h < (DISPLAY_HEIGHT+ DISPLAY_TOP); h++) {
                        cur_fb16 = (unsigned short *)((__u32)fb0 + h*fix.line_length);
                        for (w = DISPLAY_LEFT; w < DISPLAY_WIDTH + DISPLAY_LEFT; w++) {
                                cur_fb16[w] = 0x0841;
                        }
                }
        }
        else if (var.bits_per_pixel == 24) {
                for (h = DISPLAY_TOP; h < (DISPLAY_HEIGHT + DISPLAY_TOP); h++) {
                        cur_fb8 = (unsigned char *)((__u32)fb0 + h*fix.line_length);
                        for (w = DISPLAY_LEFT; w < DISPLAY_WIDTH + DISPLAY_LEFT; w++) {
                                *cur_fb8++ = 8;
                                *cur_fb8++ = 8;
                                *cur_fb8++ = 8;
                        }
                }
        }
        else if (var.bits_per_pixel == 32) {
                for (h = DISPLAY_TOP; h < (DISPLAY_HEIGHT + DISPLAY_TOP); h++) {
                        cur_fb32 = (unsigned int *)((__u32)fb0 + h*fix.line_length);
                        for (w = DISPLAY_LEFT; w < DISPLAY_WIDTH + DISPLAY_LEFT; w++) {
                                cur_fb32[w] = 0x00080808;
                        }
                }
        }
        if (ioctl(fd_v4l, VIDIOC_G_FBUF, &fb_v4l2) < 0) {
                printf("Get framebuffer failed\n");
                return TFAIL;
        }
        fb_v4l2.flags = V4L2_FBUF_FLAG_OVERLAY;

        close(fd_fb);

        if (ioctl(fd_v4l, VIDIOC_S_FBUF, &fb_v4l2) < 0)
        {
                printf("set framebuffer failed\n");
                return TFAIL;
        }

        if (ioctl(fd_v4l, VIDIOC_G_FBUF, &fb_v4l2) < 0) {
                printf("set framebuffer failed\n");
                return TFAIL;
        }

        printf("\n frame buffer width %d, height %d, bytesperline %d\n",
                fb_v4l2.fmt.width, fb_v4l2.fmt.height, fb_v4l2.fmt.bytesperline); // 800 480 1600
        ret = mxc_v4l_tvin_test();

        close(fd_v4l);
        return ret;
}

