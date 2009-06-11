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
 * @file mxc_v4l2_still.c
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
#include <errno.h>
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
#include <sys/mman.h>
#include <string.h>
#include <malloc.h>

static int g_convert = 0;
static int g_width = 640;
static int g_height = 480;
static unsigned long g_pixelformat = V4L2_PIX_FMT_YUYV;
static int g_bpp = 16;
static int g_camera_framerate = 30;
static int g_capture_mode = 0;

void usage(void)
{
	printf("Usage: mxc_v4l2_still.out [-w width] [-h height] [-f pixformat] [-c] [-m] [-fr]\n"
                "-w    Image width, 640 by default\n"
                "-h    Image height, 480 by default\n"
                "-f    Image pixel format, YUV420, YUV422P, YUYV (default), UYVY or YUV444\n"
                "-c    Convert to YUV420P. This option is valid for interleaved pixel\n"
                "      formats only - YUYV, UYVY, YUV444\n"
		"-m    Capture mode, 0-low resolution(default), 1-high resolution \n"
		"-fr   Capture frame rate, 30fps by default\n"
                "The output is saved in ./still.yuv\n"
		"MX25(CSI) driver supports RGB565, YUV420 and UYVY\n"
                "MX27(eMMA) driver supports YUV420, YUYV and YUV444\n"
                "MXC(IPU) driver supports UYVY, YUV422P and YUV420\n\n"
                );
}

/* Convert to YUV420 format */
void fmt_convert(char *dest, char *src, struct v4l2_format fmt)
{
        int row, col, pos = 0;
        int bpp, yoff, uoff, voff;

        if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
                bpp = 2;
                yoff = 0;
                uoff = 1;
                voff = 3;
        }
        else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_UYVY) {
                bpp = 2;
                yoff = 1;
                uoff = 0;
                voff = 2;
        }
        else {	/* YUV444 */
                bpp = 4;
                yoff = 0;
                uoff = 1;
                voff = 2;
        }

        /* Copy Y */
        for (row = 0; row < fmt.fmt.pix.height; row++)
                for (col = 0; col < fmt.fmt.pix.width; col++)
                        dest[pos++] = src[row * fmt.fmt.pix.bytesperline + col * bpp + yoff];

        /* Copy U */
        for (row = 0; row < fmt.fmt.pix.height; row += 2) {
                for (col = 0; col < fmt.fmt.pix.width; col += 2)
                        dest[pos++] = src[row * fmt.fmt.pix.bytesperline + col * bpp + uoff];
        }

        /* Copy V */
        for (row = 0; row < fmt.fmt.pix.height; row += 2) {
                for (col = 0; col < fmt.fmt.pix.width; col += 2)
                        dest[pos++] = src[row * fmt.fmt.pix.bytesperline + col * bpp + voff];
        }
}

int v4l_capture_setup(void)
{
        char v4l_device[100] = "/dev/video0";
        struct v4l2_streamparm parm;
        struct v4l2_format fmt;
        struct v4l2_crop crop;
        int fd_v4l = 0;

        if ((fd_v4l = open(v4l_device, O_RDWR, 0)) < 0)
        {
                printf("Unable to open %s\n", v4l_device);
                return 0;
        }

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = g_camera_framerate;
	parm.parm.capture.capturemode = g_capture_mode;

	if (ioctl(fd_v4l, VIDIOC_S_PARM, &parm) < 0)
	{
		printf("VIDIOC_S_PARM failed\n");
		return 0;
	}

	memset(&fmt, 0, sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.pixelformat = g_pixelformat;
        fmt.fmt.pix.width = g_width;
        fmt.fmt.pix.height = g_height;
        fmt.fmt.pix.sizeimage = fmt.fmt.pix.width * fmt.fmt.pix.height * g_bpp / 8;
        fmt.fmt.pix.bytesperline = g_width * g_bpp / 8;

        if (ioctl(fd_v4l, VIDIOC_S_FMT, &fmt) < 0)
        {
                printf("set format failed\n");
                return 0;
        }

        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c.left = 0;
        crop.c.top = 0;
        crop.c.width = g_width;
        crop.c.height = g_height;
        if (ioctl(fd_v4l, VIDIOC_S_CROP, &crop) < 0)
        {
                printf("set cropping failed\n");
                return 0;
        }

        return fd_v4l;
}

void v4l_capture_test(int fd_v4l)
{
        struct v4l2_format fmt;
        struct v4l2_streamparm parm;
        int fd_still = 0;
        char *buf1, *buf2;
        char still_file[100] = "./still.yuv";

        if ((fd_still = open(still_file, O_RDWR | O_CREAT | O_TRUNC)) < 0)
        {
                printf("Unable to create y frame recording file\n");
                return;
        }

        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd_v4l, VIDIOC_G_FMT, &fmt) < 0) {
                printf("get format failed\n");
                return;
        } else {
                printf("\t Width = %d\n", fmt.fmt.pix.width);
                printf("\t Height = %d\n", fmt.fmt.pix.height);
                printf("\t Image size = %d\n", fmt.fmt.pix.sizeimage);
                printf("\t Pixel format = %c%c%c%c\n",
                        (char)(fmt.fmt.pix.pixelformat & 0xFF),
                        (char)((fmt.fmt.pix.pixelformat & 0xFF00) >> 8),
                        (char)((fmt.fmt.pix.pixelformat & 0xFF0000) >> 16),
                        (char)((fmt.fmt.pix.pixelformat & 0xFF000000) >> 24));
        }

        buf1 = (char *)malloc(fmt.fmt.pix.sizeimage);
        buf2 = (char *)malloc(fmt.fmt.pix.sizeimage);
        if (!buf1 || !buf2)
                goto exit0;

        memset(buf1, 0, fmt.fmt.pix.sizeimage);
        memset(buf2, 0, fmt.fmt.pix.sizeimage);

        if (read(fd_v4l, buf1, fmt.fmt.pix.sizeimage) != fmt.fmt.pix.sizeimage) {
                printf("v4l2 read error.\n");
                goto exit0;
        }

        if ((g_convert == 1) && (g_pixelformat != V4L2_PIX_FMT_YUV422P)
        	&& (g_pixelformat != V4L2_PIX_FMT_YUV420)) {
                fmt_convert(buf2, buf1, fmt);
                write(fd_still, buf2, fmt.fmt.pix.width * fmt.fmt.pix.height * 3 / 2);
        }
        else
                write(fd_still, buf1, fmt.fmt.pix.sizeimage);

exit0:
        if (buf1)
                free(buf1);
        if (buf2)
                free(buf2);
        close(fd_still);
        close(fd_v4l);
}

int main(int argc, char **argv)
{
        int fd_v4l;
        int i;

        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-w") == 0) {
                        g_width = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-h") == 0) {
                        g_height = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-c") == 0) {
                        g_convert = 1;
                }
		else if (strcmp(argv[i], "-m") == 0) {
			g_capture_mode = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-fr") == 0) {
			g_camera_framerate = atoi(argv[++i]);
		}
                else if (strcmp(argv[i], "-f") == 0) {
                        i++;
                        if (strcmp(argv[i], "YUV420") == 0) {
                                g_pixelformat = V4L2_PIX_FMT_YUV420;
                                g_bpp = 12;
                        }
                        else if (strcmp(argv[i], "YUV422P") == 0) {
                                g_pixelformat = V4L2_PIX_FMT_YUV422P;
                                g_bpp = 16;
                        }
                        else if (strcmp(argv[i], "YUYV") == 0) {
                                g_pixelformat = V4L2_PIX_FMT_YUYV;
                                g_bpp = 16;
                        }
                        else if (strcmp(argv[i], "UYVY") == 0) {
                                g_pixelformat = V4L2_PIX_FMT_UYVY;
                                g_bpp = 16;
                        }
                        else if (strcmp(argv[i], "YUV444") == 0) {
                                g_pixelformat = v4l2_fourcc('Y','4','4','4');
                                g_bpp = 32;
                        }
                        else if (strcmp(argv[i], "RGB565") == 0) {
                                g_pixelformat = V4L2_PIX_FMT_RGB565;
                                g_bpp = 16;
                        }
                        else {
                                printf("Pixel format not supported.\n");
                                usage();
                                return 0;
                        }
                }
                else {
                        usage();
                        return 0;
                }
        }

        fd_v4l = v4l_capture_setup();
        v4l_capture_test(fd_v4l);

        return 0;
}
