/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All rights reserved.
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
 * @file mxc_v4l2_overlay.c
 *
 * @brief Mxc Video For Linux 2 driver test application
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
#include <linux/compiler.h>
#include <linux/videodev.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>

#include <asm/arch/mxcfb.h>

#define TFAIL -1
#define TPASS 0

#define ipu_fourcc(a,b,c,d)\
        (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))

#define IPU_PIX_FMT_RGB332  ipu_fourcc('R','G','B','1') /*!<  8  RGB-3-3-2     */
#define IPU_PIX_FMT_RGB555  ipu_fourcc('R','G','B','O') /*!< 16  RGB-5-5-5     */
#define IPU_PIX_FMT_RGB565  ipu_fourcc('R','G','B','P') /*!< 16  RGB-5-6-5     */
#define IPU_PIX_FMT_RGB666  ipu_fourcc('R','G','B','6') /*!< 18  RGB-6-6-6     */
#define IPU_PIX_FMT_BGR24   ipu_fourcc('B','G','R','3') /*!< 24  BGR-8-8-8     */
#define IPU_PIX_FMT_RGB24   ipu_fourcc('R','G','B','3') /*!< 24  RGB-8-8-8     */
#define IPU_PIX_FMT_BGR32   ipu_fourcc('B','G','R','4') /*!< 32  BGR-8-8-8-8   */
#define IPU_PIX_FMT_BGRA32  ipu_fourcc('B','G','R','A') /*!< 32  BGR-8-8-8-8   */
#define IPU_PIX_FMT_RGB32   ipu_fourcc('R','G','B','4') /*!< 32  RGB-8-8-8-8   */
#define IPU_PIX_FMT_RGBA32  ipu_fourcc('R','G','B','A') /*!< 32  RGB-8-8-8-8   */
#define IPU_PIX_FMT_ABGR32  ipu_fourcc('A','B','G','R') /*!< 32  ABGR-8-8-8-8  */

char v4l_device[100] = "/dev/video0";
int fd_v4l = 0;
int g_sensor_width = 640;
int g_sensor_height = 480;
int g_sensor_top = 0;
int g_sensor_left = 0;
int g_display_width = 240;
int g_display_height = 320;
int g_display_top = 0;
int g_display_left = 0;
int g_rotate = 0;
int g_timeout = 3600;
int g_display_lcd = 0;
int g_overlay = 0;
int g_camera_color = 0;
int g_camera_framerate = 30;
int g_capture_mode = 0;

int
mxc_v4l_overlay_test(int timeout)
{
        int i;
        int overlay = 1;
        struct v4l2_control ctl;

        if (ioctl(fd_v4l, VIDIOC_OVERLAY, &overlay) < 0)
        {
                printf("VIDIOC_OVERLAY start failed\n");
		return TFAIL;
        }

        for (i = 0; i < 3 ; i++) {
                // flash a frame
                ctl.id = V4L2_CID_PRIVATE_BASE + 1;
                if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl) < 0)
                {
                        printf("set ctl failed\n");
			return TFAIL;
                }
	            sleep(1);
        }

        if (g_camera_color == 1) {
                ctl.id = V4L2_CID_BRIGHTNESS;
                for (i = 0; i < 0xff; i+=0x20) {
		            ctl.value = i;
                	printf("change the brightness %d\n", i);
                    ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
		            sleep(1);
                }
		}
		else if (g_camera_color == 2) {
                ctl.id = V4L2_CID_SATURATION;
                for (i = 25; i < 150; i+= 25) {
		            ctl.value = i;
	                printf("change the color saturation %d\n", i);
                    ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
		            sleep(5);
                }
		}
		else if (g_camera_color == 3) {
                ctl.id = V4L2_CID_RED_BALANCE;
                for (i = 0; i < 0xff; i+=0x20) {
		            ctl.value = i;
	                printf("change the red balance %d\n", i);
                    ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
		            sleep(1);
                }
		}
		else if (g_camera_color == 4) {
                ctl.id = V4L2_CID_BLUE_BALANCE;
                for (i = 0; i < 0xff; i+=0x20) {
		            ctl.value = i;
                	printf("change the blue balance %d\n", i);
                    ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
		            sleep(1);
                }
		}
		else if (g_camera_color == 5) {
                ctl.id = V4L2_CID_BLACK_LEVEL;
                for (i = 0; i < 4; i++) {
		            ctl.value = i;
                	printf("change the black balance %d\n", i);
                    ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
		            sleep(5);
                }
        }
        else {
		        sleep(timeout);
        }

        overlay = 0;
        if (ioctl(fd_v4l, VIDIOC_OVERLAY, &overlay) < 0)
        {
                printf("VIDIOC_OVERLAY stop failed\n");
		return TFAIL;
        }
	return 0;
}

int
mxc_v4l_overlay_setup(struct v4l2_format *fmt)
{
        struct v4l2_streamparm parm;
        v4l2_std_id id;
        struct v4l2_control ctl;
        struct v4l2_crop crop;

        if (ioctl(fd_v4l, VIDIOC_S_OUTPUT, &g_display_lcd) < 0)
        {
                printf("VIDIOC_S_OUTPUT failed\n");
                return TFAIL;
        }

        ctl.id = V4L2_CID_PRIVATE_BASE;
		ctl.value = g_rotate;
        if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl) < 0)
        {
                printf("set control failed\n");
                return TFAIL;
        }

        crop.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
        crop.c.left = g_sensor_left;
        crop.c.top = g_sensor_top;
        crop.c.width = g_sensor_width;
        crop.c.height = g_sensor_height;
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

        if (ioctl(fd_v4l, VIDIOC_G_STD, &id) < 0)
        {
                printf("VIDIOC_G_STD failed\n");
                return TFAIL;
        }

        parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        parm.parm.capture.timeperframe.numerator = 1;
        parm.parm.capture.timeperframe.denominator = g_camera_framerate;
	parm.parm.capture.capturemode = g_capture_mode;

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

        printf("frame_rate is %d\n", parm.parm.capture.timeperframe.denominator);
        return TPASS;
}

int process_cmdline(int argc, char **argv)
{
        int i;

        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-iw") == 0) {
                        g_sensor_width = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-ih") == 0) {
                        g_sensor_height = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-it") == 0) {
                        g_sensor_top = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-il") == 0) {
                        g_sensor_left = atoi(argv[++i]);
                }
                if (strcmp(argv[i], "-ow") == 0) {
                        g_display_width = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-oh") == 0) {
                        g_display_height = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-ot") == 0) {
                        g_display_top = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-ol") == 0) {
                        g_display_left = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-r") == 0) {
                        g_rotate = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-t") == 0) {
                        g_timeout = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-d") == 0) {
                        g_display_lcd = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-fg") == 0) {
                        g_overlay = 1;
                }
                else if (strcmp(argv[i], "-v") == 0) {
                        g_camera_color = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-fr") == 0) {
                        g_camera_framerate = atoi(argv[++i]);
                }
		else if (strcmp(argv[i], "-m") == 0) {
			g_capture_mode = atoi(argv[++i]);
		}
                else if (strcmp(argv[i], "-help") == 0) {
                        printf("MXC Video4Linux overlay Device Test\n\n" \
                               " -iw <input width>\n -ih <input height>\n" \
                               " -it <input top>\n -il <input left>\n"	\
                               " -ow <display width>\n -oh <display height>\n" \
                               " -ot <display top>\n -ol <display left>\n"	\
                               " -r <rotate mode>\n -t <timeout>\n" \
                               " -d <output display> \n"	\
                               " -v <camera color> 1-brightness 2-saturation"
                               " 3-red 4-blue 5-black balance\n"\
			       " -m <capture mode> 0-low resolution 1-high resolution\n" \
			       " -fr <frame rate> 30fps by default\n" \
                               " -fg foreground mode when -fg specified,"
                               " otherwise go to frame buffer\n");
                        return -1;
                }
        }

        printf("g_display_width = %d, g_display_height = %d\n", g_display_width, g_display_height);
        printf("g_display_top = %d, g_display_left = %d\n", g_display_top, g_display_left);

        if ((g_display_width == 0) || (g_display_height == 0)) {
                return -1;
        }
        return 0;
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

        if (process_cmdline(argc, argv) < 0) {
                return TFAIL;
        }

        if ((fd_v4l = open(v4l_device, O_RDWR, 0)) < 0) {
                printf("Unable to open %s\n", v4l_device);
                return TFAIL;
        }

        fmt.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
        fmt.fmt.win.w.top=  g_display_top ;
        fmt.fmt.win.w.left= g_display_left;
        fmt.fmt.win.w.width=g_display_width;
        fmt.fmt.win.w.height=g_display_height;

        if (mxc_v4l_overlay_setup(&fmt) < 0) {
                printf("Setup overlay failed.\n");
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

        if (!g_overlay) {
                fb_v4l2.fmt.width = var.xres;
                fb_v4l2.fmt.height = var.yres;
                if (var.bits_per_pixel == 32) {
                        fb_v4l2.fmt.pixelformat = IPU_PIX_FMT_BGR32;
                        fb_v4l2.fmt.bytesperline = 4 * fb_v4l2.fmt.width;
                }
                else if (var.bits_per_pixel == 24) {
                        fb_v4l2.fmt.pixelformat = IPU_PIX_FMT_BGR24;
                        fb_v4l2.fmt.bytesperline = 3 * fb_v4l2.fmt.width;
                }
                else if (var.bits_per_pixel == 16) {
                        fb_v4l2.fmt.pixelformat = IPU_PIX_FMT_RGB565;
                        fb_v4l2.fmt.bytesperline = 2 * fb_v4l2.fmt.width;
                }

                fb_v4l2.flags = V4L2_FBUF_FLAG_PRIMARY;
                fb_v4l2.base = (void *) fix.smem_start;
        } else {

	        alpha.alpha = 255;
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

	        screen_size = var.yres * fix.line_length;

	        /* Map the device to memory*/
	        fb0 = (unsigned short *)mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
	        if ((int)fb0 == -1)
	        {
	                printf("\nError: failed to map framebuffer device 0 to memory.\n");
                        close(fd_fb);
                        return TFAIL;
	        }


	        if (var.bits_per_pixel == 16) {
	                for (h = g_display_top; h < (g_display_height + g_display_top); h++) {
	                        cur_fb16 = (unsigned short *)((__u32)fb0 + h*fix.line_length);
	                        for (w = g_display_left; w < g_display_width + g_display_left; w++) {
	                                cur_fb16[w] = 0x0841;
	                        }
	                }
	        }
	        else if (var.bits_per_pixel == 24) {
	                for (h = g_display_top; h < (g_display_height + g_display_top); h++) {
	                        cur_fb8 = (unsigned char *)((__u32)fb0 + h*fix.line_length);
	                        for (w = g_display_left; w < g_display_width + g_display_left; w++) {
	                                *cur_fb8++ = 8;
	                                *cur_fb8++ = 8;
	                                *cur_fb8++ = 8;
	                        }
	                }
	        }
	        else if (var.bits_per_pixel == 32) {
	                for (h = g_display_top; h < (g_display_height + g_display_top); h++) {
	                        cur_fb32 = (unsigned int *)((__u32)fb0 + h*fix.line_length);
	                        for (w = g_display_left; w < g_display_width + g_display_left; w++) {
	                                cur_fb32[w] = 0x00080808;
	                        }
	                }
	        }
                if (ioctl(fd_v4l, VIDIOC_G_FBUF, &fb_v4l2) < 0) {
                        printf("Get framebuffer failed\n");
                        return TFAIL;
                }
                fb_v4l2.flags = V4L2_FBUF_FLAG_OVERLAY;
        }

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
                fb_v4l2.fmt.width, fb_v4l2.fmt.height, fb_v4l2.fmt.bytesperline);
        ret = mxc_v4l_overlay_test(g_timeout);

        close(fd_v4l);
        return ret;
}

