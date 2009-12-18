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
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/videodev.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>

#include <linux/mxcfb.h>
//#include <linux/mxc_v4l2.h>

struct v4l2_mxc_offset {
	uint32_t u_offset;
	uint32_t v_offset;
};

#define TFAIL -1
#define TPASS 0

char v4l_device[100] = "/dev/video16";
int fd_v4l = 0;
int g_frame_size;
int g_num_buffers;

int g_output = 3;
int g_overlay = 0;
int g_in_width = 0;
int g_in_height = 0;
int g_display_width = 0;
int g_display_height = 0;
int g_display_top = 0;
int g_display_left = 0;
int g_rotate = 0;
int g_extra_pixel = 0;
uint32_t g_in_fmt = V4L2_PIX_FMT_YUV420;

int g_bmp_header = 0;
int g_loop_count = 1;
int g_frame_period = 33333;

#define MAX_BUFFER_NUM 5

struct testbuffer
{
        unsigned char *start;
        size_t offset;
        unsigned int length;
};

struct testbuffer buffers[5];

/*
       Y = R *  .299 + G *  .587 + B *  .114;
       U = R * -.169 + G * -.332 + B *  .500 + 128.;
       V = R *  .500 + G * -.419 + B * -.0813 + 128.;*/

#define red(x) (((x & 0xE0) >> 5) * 0x24)
#define green(x) (((x & 0x1C) >> 2) * 0x24)
#define blue(x) ((x & 0x3) * 0x55)
#define y(rgb) ((red(rgb)*299L + green(rgb)*587L + blue(rgb)*114L) / 1000)
#define u(rgb) ((((blue(rgb)*500L) - (red(rgb)*169L) - (green(rgb)*332L)) / 1000))
#define v(rgb) (((red(rgb)*500L - green(rgb)*419L - blue(rgb)*81L) / 1000))

void gen_fill_pattern(char * buf, int frame_num)
{
	int y_size = g_in_width * g_in_height;
	int h_step = g_in_height / 16;
	int w_step = g_in_width / 16;
	int h, w;
	uint32_t y_color = 0;
	int32_t u_color = 0;
	int32_t v_color = 0;
	uint32_t rgb = 0;
	static int32_t alpha = 0;
	static int inc_alpha = 1;

	for (h = 0; h < g_in_height; h++) {
		int32_t rgb_temp = rgb;

		for (w = 0; w < g_in_width; w++) {
			if (w % w_step == 0) {
				y_color = y(rgb_temp);
				y_color = (y_color * alpha) / 255;

				u_color = u(rgb_temp);
				u_color = (u_color * alpha) / 255;
				u_color += 128;

				v_color = v(rgb_temp);
				v_color = (v_color * alpha) / 255;
				v_color += 128;

				rgb_temp++;
				if (rgb_temp > 255)
					rgb_temp = 0;
//				printf("y = %8d, u = %08x, v = %08x\n", y_color, u_color, v_color);
			}
			buf[(h*g_in_width) + w] = y_color;
			if (!(h & 0x1) && !(w & 0x1)) {
				buf[y_size + (((h*g_in_width)/4) + (w/2)) ] = u_color;
				buf[y_size + y_size/4 + (((h*g_in_width)/4) + (w/2))] = v_color;
			}
		}
		if ((h > 0) && (h % h_step == 0)) {
			rgb += 16;
			if (rgb > 255)
				rgb = 0;
		}

	}
	if (inc_alpha) {
		alpha+=4;
		if (alpha >= 255) {
			inc_alpha = 0;
		}
	} else {
		alpha-=4;
		if (alpha <= 0) {
			inc_alpha = 1;
		}
	}
}

int
mxc_v4l_output_test(FILE *in)
{
        int i;
        int err = 0;
        int retval = 0;
        int type;
        struct v4l2_buffer buf;
        int y_size = (g_frame_size * 2) / 3;
        __u32 total_time;
	int count = 100;
        struct timeval tv_start, tv_current;

        memset(&buf, 0, sizeof(buf));

        for (i = 0; i < g_num_buffers; i++)
        {
                memset(&buf, 0, sizeof (buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                if (ioctl(fd_v4l, VIDIOC_QUERYBUF, &buf) < 0)
                {
                        printf("VIDIOC_QUERYBUF error\n");
			retval = -1;
                        goto cleanup;
                }

                buffers[i].length = buf.length;
                buffers[i].offset = (size_t) buf.m.offset;
                printf("VIDIOC_QUERYBUF: length = %d, offset = %d\n",  buffers[i].length, buffers[i].offset);
                buffers[i].start = mmap (NULL, buffers[i].length,
                                         PROT_READ | PROT_WRITE, MAP_SHARED,
                                         fd_v4l, buffers[i].offset);
                if (buffers[i].start == NULL) {
                        printf("v4l2_out test: mmap failed\n");
			retval = -1;
                }
        }

        gettimeofday(&tv_start, 0);
        printf("start time = %d s, %d us\n", tv_start.tv_sec, tv_start.tv_usec);

        for (i = 0; ; i++) {
                buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                buf.memory = V4L2_MEMORY_MMAP;
                if (i < g_num_buffers) {
                        buf.index = i;
                        if (ioctl(fd_v4l, VIDIOC_QUERYBUF, &buf) < 0)
                        {
                                printf("VIDIOC_QUERYBUF failed\n");
				retval = -1;
                                break;
                        }
                }
                else {
                        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                        buf.memory = V4L2_MEMORY_MMAP;
                        if (ioctl(fd_v4l, VIDIOC_DQBUF, &buf) < 0)
                        {
                                printf("VIDIOC_DQBUF failed\n");
				retval = -1;
                                break;
                        }
                }
                if (in) {
                        // skip over bmp header in each frame
                        if (g_bmp_header) {
                                fseek(in, 0x36, SEEK_CUR);
                        }

                        err = fread(buffers[buf.index].start, 1, g_frame_size, in);
                        if (err < g_frame_size) {
                                g_loop_count--;
                                if (g_loop_count == 0) {
                                        printf("v4l2_output: end of input file, g_frame_size=%d, err = %d\n", g_frame_size, err);
                                        break;
                                }
                                fseek(in, 0, SEEK_SET);
                                if (g_bmp_header) {
                                        fseek(in, 0x36, SEEK_CUR);
                                }
                                err = fread(buffers[buf.index].start, 1, g_frame_size, in);
                        }
                }
                else {
			if (count-- == 0) {
				if (g_loop_count-- == 0) {
					break;
				}
				count = 100;
			}
                        gen_fill_pattern(buffers[buf.index].start, i);
                }
                //sleep(1);

                buf.timestamp.tv_sec = tv_start.tv_sec;
                buf.timestamp.tv_usec = tv_start.tv_usec + (g_frame_period * i);
                //printf("buffer timestamp = %d s, %d us\n", buf.timestamp.tv_sec, buf.timestamp.tv_usec);
                if (g_extra_pixel) {
                	buf.m.offset = buffers[buf.index].offset + g_extra_pixel *
                		(g_in_width + 2 * g_extra_pixel) + g_extra_pixel;
				}
                if (ioctl(fd_v4l, VIDIOC_QBUF, &buf) < 0)
                {
                        printf("VIDIOC_QBUF failed\n");
			retval = -1;
                        break;
                }

                if ( i == 1 ) { // Start playback after buffers queued
                        type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                        if (ioctl (fd_v4l, VIDIOC_STREAMON, &type) < 0) {
                                printf("Could not start stream\n");
				retval = -1;
                                break;
                        }
                }
        }

        gettimeofday(&tv_current, 0);
        total_time = (tv_current.tv_sec - tv_start.tv_sec) * 1000000L;
        total_time += tv_current.tv_usec - tv_start.tv_usec;
        printf("total time for %u frames = %u us =  %lld fps\n", i, total_time, (i * 1000000ULL) / total_time);

        type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        ioctl (fd_v4l, VIDIOC_STREAMOFF, &type);

cleanup:
        for (i = 0; i < g_num_buffers; i++)
        {
                munmap (buffers[i].start, buffers[i].length);
        }
	return retval;
}

int
mxc_v4l_output_setup(struct v4l2_format *fmt)
{
        struct v4l2_requestbuffers buf_req;

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

        memset(&buf_req, 0, sizeof(buf_req));
        buf_req.count = 4;
        buf_req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf_req.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd_v4l, VIDIOC_REQBUFS, &buf_req) < 0)
        {
                printf("request buffers failed\n");
                return TFAIL;
        }
        g_num_buffers = buf_req.count;
        printf("v4l2_output test: Allocated %d buffers\n", buf_req.count);

        return TPASS;
}


int fb_setup(void)
{
        int i;
        char fbdev[] = "/dev/fb0";
        struct fb_var_screeninfo fb_var;
        struct fb_fix_screeninfo fb_fix;
        struct mxcfb_color_key color_key;
        struct mxcfb_gbl_alpha alpha;
        int retval = -1;
        int fd_fb;
        unsigned short * fb0;
        unsigned char * cur_fb8;
        unsigned short * cur_fb16;
        unsigned int * cur_fb32;
        __u32 screen_size;
        int h, w;

        for (i=0; i < 5; i++) {
                fbdev[7] = '0' + i;
                if ((fd_fb = open(fbdev, O_RDWR, 0)) < 0)
                {
                        printf("Unable to open %s\n", fbdev);
                        retval = TFAIL;
                        goto err0;
                }
                if ( ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix) < 0) {
                        goto err1;
                }
		if ((g_output == 5) && (strcmp(fb_fix.id, "DISP3 BG - DI1") == 0)) {
			break;
		} else if ((g_output == 4) && (strcmp(fb_fix.id, "DISP3 BG") == 0)) {
			break;
		} else if ((g_output == 3) && (strcmp(fb_fix.id, "DISP3 FG") == 0)) {
			break;
		} else if (fb_fix.id[4] == ('0' + g_output)) {
                        break;
		}
                close(fd_fb);
        }

        if ( ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var) < 0) {
                goto err1;
        }
        if (g_display_height == 0)
                g_display_height = fb_var.yres;

        if (g_display_width == 0)
                g_display_width = fb_var.xres;

        if ((g_display_top + g_display_height) > fb_var.yres)
                g_display_height = fb_var.yres - g_display_top;
        if ((g_display_left + g_display_width) > fb_var.xres)
                g_display_width = fb_var.xres - g_display_left;

        alpha.alpha = 255;
        alpha.enable = 1;
        if ( ioctl(fd_fb, MXCFB_SET_GBL_ALPHA, &alpha) < 0) {
                retval = 0; // No error, fails for ADC panels
                goto err1;
        }

        color_key.color_key = 0x00080808;
        color_key.enable = 1;
        if ( ioctl(fd_fb, MXCFB_SET_CLR_KEY, &color_key) < 0) {
                retval = 0; // No error, fails for ADC panels
                goto err1;
        }

        screen_size = fb_var.yres * fb_fix.line_length;

        /* Map the device to memory*/
        fb0 = (unsigned short *)mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
        if ((int)fb0 == -1)
        {
                printf("\nError: failed to map framebuffer device 0 to memory.\n");
                retval = -1;
                goto err1;
        }


        if (fb_var.bits_per_pixel == 16) {
                for (h = g_display_top; h < (g_display_height + g_display_top); h++) {
                        cur_fb16 = (unsigned short *)((__u32)fb0 + h*fb_fix.line_length);
                        for (w = g_display_left; w < g_display_width + g_display_left; w++) {
                                cur_fb16[w] = 0x0841;
                        }
                }
        }
        else if (fb_var.bits_per_pixel == 24) {
                for (h = g_display_top; h < (g_display_height + g_display_top); h++) {
                        cur_fb8 = (unsigned char *)((__u32)fb0 + h*fb_fix.line_length);
                        for (w = g_display_left; w < g_display_width + g_display_left; w++) {
                                *cur_fb8++ = 8;
                                *cur_fb8++ = 8;
                                *cur_fb8++ = 8;
                        }
                }
        }
        else if (fb_var.bits_per_pixel == 32) {
                for (h = g_display_top; h < (g_display_height + g_display_top); h++) {
                        cur_fb32 = (unsigned int *)((__u32)fb0 + h*fb_fix.line_length);
                        for (w = g_display_left; w < g_display_width + g_display_left; w++) {
                                cur_fb32[w] = 0x00080808;
                        }
                }
        }

        retval = 0;
//        munmap(fb0, screen_size);
err1:
        close(fd_fb);
err0:
        return retval;
}



int process_cmdline(int argc, char **argv)
{
        int i;

        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-iw") == 0) {
                        g_in_width = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-ih") == 0) {
                        g_in_height = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-ow") == 0) {
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
                else if (strcmp(argv[i], "-d") == 0) {
                        g_output = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-fg") == 0) {
                        g_overlay = 1;
                }
                else if (strcmp(argv[i], "-r") == 0) {
                        g_rotate = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-fr") == 0) {
                        g_frame_period = atoi(argv[++i]);
                        g_frame_period = 1000000L / g_frame_period;
                }
                else if (strcmp(argv[i], "-l") == 0) {
                        g_loop_count = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-e") == 0) {
                        g_extra_pixel = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-f") == 0) {
                        i++;
                        g_in_fmt = v4l2_fourcc(argv[i][0], argv[i][1],argv[i][2],argv[i][3]);

                        if ( (g_in_fmt != V4L2_PIX_FMT_BGR24) &&
                             (g_in_fmt != V4L2_PIX_FMT_BGR32) &&
                             (g_in_fmt != V4L2_PIX_FMT_RGB565) &&
                             (g_in_fmt != 'PMBW') &&
                             (g_in_fmt != V4L2_PIX_FMT_UYVY) &&
			     (g_in_fmt != V4L2_PIX_FMT_YUV422P) &&
                             (g_in_fmt != V4L2_PIX_FMT_YUV420) &&
                             (g_in_fmt != V4L2_PIX_FMT_NV12) )
                        {
                                return -1;
                        }
                        if (g_in_fmt == 'PMBW') {
                                g_bmp_header = 1;
                                g_in_fmt = V4L2_PIX_FMT_BGR24;
                        }
                }
        }

        printf("g_in_width = %d, g_in_height = %d\n", g_in_width, g_in_height);
        printf("g_display_width = %d, g_display_height = %d\n", g_display_width, g_display_height);

        if ((g_in_width == 0) || (g_in_height == 0)) {
                return -1;
        }
        return 0;
}

int
main(int argc, char **argv)
{
        struct v4l2_control ctrl;
        struct v4l2_format fmt;
        struct v4l2_framebuffer fb;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        FILE * fd_in;
        int retval = TPASS;
		struct v4l2_mxc_offset off;

        if (process_cmdline(argc, argv) < 0) {
                printf("MXC Video4Linux Output Device Test\n\n" \
                       "Syntax: mxc_v4l2_output.out\n -d <output display #>\n" \
                       "-iw <input width> -ih <input height>\n [-f <WBMP|fourcc code>]" \
                       "[-ow <display width>\n -oh <display height>]\n" \
                       " -e <input cropping: extra pixels> \n" \
                       "[-fr <frame rate (fps)>] [-r <rotate mode 0-7>] [-l <loop count>]\n" \
                       "<input YUV file>\n\n");
                retval = TFAIL;
                goto err0;
        }

        if (fb_setup() < 0) {
                retval = TFAIL;
                goto err1;
        }

        if ((fd_v4l = open(v4l_device, O_RDWR, 0)) < 0)
        {
                printf("Unable to open %s\n", v4l_device);
                retval = TFAIL;
                goto err0;
        }

        if (ioctl(fd_v4l, VIDIOC_S_OUTPUT, &g_output) < 0)
        {
                printf("set output failed\n");
                return TFAIL;
        }

        memset(&cropcap, 0, sizeof(cropcap));
        cropcap.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        if (ioctl(fd_v4l, VIDIOC_CROPCAP, &cropcap) < 0)
        {
                printf("get crop capability failed\n");
                retval = TFAIL;
                goto err1;
        }
        printf("cropcap.bounds.width = %d\ncropcap.bound.height = %d\n" \
               "cropcap.defrect.width = %d\ncropcap.defrect.height = %d\n",
               cropcap.bounds.width, cropcap.bounds.height,
               cropcap.defrect.width, cropcap.defrect.height);

        crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        crop.c.top = g_display_top;
        crop.c.left = g_display_left;
        crop.c.width = g_display_width;
        crop.c.height = g_display_height;
        if (ioctl(fd_v4l, VIDIOC_S_CROP, &crop) < 0)
        {
                printf("set crop failed\n");
                retval = TFAIL;
                goto err1;
        }

        // Set rotation
        ctrl.id = V4L2_CID_PRIVATE_BASE;
        ctrl.value = g_rotate;
        if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
        {
                printf("set ctrl failed\n");
                retval = TFAIL;
                goto err1;
        }

        // Set framebuffer parameter for MX27
        fb.capability = V4L2_FBUF_CAP_EXTERNOVERLAY;
        if (g_overlay)
                fb.flags = V4L2_FBUF_FLAG_OVERLAY;
        else
                fb.flags = V4L2_FBUF_FLAG_PRIMARY;
        ioctl(fd_v4l, VIDIOC_S_FBUF, &fb);

        memset(&fmt, 0, sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        fmt.fmt.pix.width=g_in_width;
        fmt.fmt.pix.height=g_in_height;
        fmt.fmt.pix.pixelformat = g_in_fmt;
		if (g_extra_pixel) {
			off.u_offset = (2 * g_extra_pixel + g_in_width) * (g_in_height + g_extra_pixel)
				 - g_extra_pixel + (g_extra_pixel / 2) * ((g_in_width / 2)
				 + g_extra_pixel) + g_extra_pixel / 2;
			off.v_offset = off.u_offset + (g_extra_pixel + g_in_width / 2) *
				((g_in_height / 2) + g_extra_pixel);
        	fmt.fmt.pix.bytesperline = g_in_width + g_extra_pixel * 2;
			fmt.fmt.pix.priv = (uint32_t) &off;
        	fmt.fmt.pix.sizeimage = (g_in_width + g_extra_pixel * 2 )
        		* (g_in_height + g_extra_pixel * 2) * 3 / 2;
        } else {
        	fmt.fmt.pix.bytesperline = g_in_width;
			fmt.fmt.pix.priv = 0;
        	fmt.fmt.pix.sizeimage = 0;
		}
        mxc_v4l_output_setup(&fmt);
        g_frame_size = fmt.fmt.pix.sizeimage;

        fd_in = fopen(argv[argc-1], "rb");
/*
        if (fd_in == NULL) {
                printf("Unable to open file: %s\n", argv[argc-1]);
                retval = TFAIL;
                goto err1;
        }
        */
        if (fd_in)
                fseek(fd_in, 0, SEEK_SET);

        retval = mxc_v4l_output_test(fd_in);

        if (fd_in)
                fclose(fd_in);
err1:
        close(fd_v4l);
err0:
        return retval;
}

