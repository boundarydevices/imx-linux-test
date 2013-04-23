/*
 * Copyright 2004-2013 Freescale Semiconductor, Inc. All rights reserved.
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
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>

#include <linux/mxcfb.h>
#include <linux/mxc_v4l2.h>
#include <linux/ipu.h>

#define TFAIL -1
#define TPASS 0

int fd_v4l = 0;
int fd_ipu = 0;
int g_frame_size;
int g_num_buffers;

char * g_dev = "/dev/video17";
int g_overlay = 0;
int g_mem_type = V4L2_MEMORY_MMAP;
int g_in_width = 0;
int g_in_height = 0;
int g_display_width = 0;
int g_display_height = 0;
int g_display_top = 0;
int g_display_left = 0;
int g_rotate = 0;
int g_vflip = 0;
int g_hflip = 0;
int g_vdi_enable = 0;
int g_vdi_motion = 0;
int g_icrop_w = 0;
int g_icrop_h = 0;
int g_icrop_top = 0;
int g_icrop_left = 0;
uint32_t g_in_fmt = V4L2_PIX_FMT_YUV420;

int g_bmp_header = 0;
int g_loop_count = 1;
int g_frame_period = 33333;

#define MAX_BUFFER_NUM 16

struct testbuffer
{
        unsigned char *start;
        size_t offset;
        unsigned int length;
};

struct testbuffer buffers[MAX_BUFFER_NUM];

void fb_setup(void)
{
        struct mxcfb_gbl_alpha alpha;
	int fd;

#ifdef BUILD_FOR_ANDROID
	fd = open("/dev/graphics/fb0",O_RDWR);
#else
	fd = open("/dev/fb0",O_RDWR);
#endif

        alpha.alpha = 0;
        alpha.enable = 1;
        if (ioctl(fd, MXCFB_SET_GBL_ALPHA, &alpha) < 0) {
		printf("set alpha %d failed for fb0\n",  alpha.alpha);
        }

	close(fd);
}

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

void gen_fill_pattern(unsigned char * buf, int frame_num)
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
        __u32 total_time;
	int count = 100;
	int streamon_cnt = 0;
        struct timeval tv_start, tv_current;

	if (g_vdi_enable)
		streamon_cnt = 1;

        memset(&buf, 0, sizeof(buf));

	if (g_mem_type == V4L2_MEMORY_MMAP) {
		for (i = 0; i < g_num_buffers; i++) {
			memset(&buf, 0, sizeof (buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			buf.memory = g_mem_type;
			buf.index = i;
			if (ioctl(fd_v4l, VIDIOC_QUERYBUF, &buf) < 0)
			{
				printf("VIDIOC_QUERYBUF error\n");
				retval = -1;
				goto cleanup;
			}

			buffers[i].length = buf.length;
			buffers[i].offset = (size_t) buf.m.offset;
			printf("VIDIOC_QUERYBUF: length = %d, offset = %d\n",
				buffers[i].length, buffers[i].offset);
			buffers[i].start = mmap (NULL, buffers[i].length,
					PROT_READ | PROT_WRITE, MAP_SHARED,
					fd_v4l, buffers[i].offset);
			if (buffers[i].start == NULL) {
				printf("v4l2_out test: mmap failed\n");
				retval = -1;
				goto cleanup;
			}
		}
	}

        gettimeofday(&tv_start, 0);
        printf("start time = %d s, %d us\n", tv_start.tv_sec, tv_start.tv_usec);

        for (i = 0; ; i++) {
                buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                buf.memory = g_mem_type;
                if (i < g_num_buffers) {
                        buf.index = i;
                        if (ioctl(fd_v4l, VIDIOC_QUERYBUF, &buf) < 0)
                        {
                                printf("VIDIOC_QUERYBUF failed\n");
				retval = -1;
                                break;
                        }
			if (g_mem_type == V4L2_MEMORY_USERPTR) {
				buf.m.userptr = (unsigned long) buffers[i].offset;
				buf.length = buffers[i].length;
			}
                }
                else {
                        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                        buf.memory = g_mem_type;
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
					if (i <= streamon_cnt)
						printf("v4l2_output: no display because v4l need at least %d frames\n", streamon_cnt + 1);
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
					if (i <= streamon_cnt)
						printf("v4l2_output: no display because v4l need at least %d frames\n", streamon_cnt + 1);
					break;
				}
				count = 30;
			}
                        gen_fill_pattern(buffers[buf.index].start, i);
                }

                buf.timestamp.tv_sec = tv_start.tv_sec;
                buf.timestamp.tv_usec = tv_start.tv_usec + (g_frame_period * i);
		if (g_vdi_enable)
			buf.field = V4L2_FIELD_INTERLACED_TB;
                if ((retval = ioctl(fd_v4l, VIDIOC_QBUF, &buf)) < 0)
                {
                        printf("VIDIOC_QBUF failed %d\n", retval);
			retval = -1;
                        break;
                }

                if ( i == streamon_cnt ) { // Start playback after buffers queued
                        type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                        if (ioctl (fd_v4l, VIDIOC_STREAMON, &type) < 0) {
                                printf("Could not start stream\n");
				retval = -1;
                                break;
                        }
			/*simply set fb0 alpha to 0*/
			fb_setup();
                }
        }

        gettimeofday(&tv_current, 0);
        total_time = (tv_current.tv_sec - tv_start.tv_sec) * 1000000L;
        total_time += tv_current.tv_usec - tv_start.tv_usec;
        printf("total time for %u frames = %u us =  %lld fps\n", i, total_time, (i * 1000000ULL) / total_time);

        type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        ioctl (fd_v4l, VIDIOC_STREAMOFF, &type);

cleanup:
	if (g_mem_type == V4L2_MEMORY_MMAP) {
		for (i = 0; i < g_num_buffers; i++) {
			munmap (buffers[i].start, buffers[i].length);
		}
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
        buf_req.memory = g_mem_type;
        if (ioctl(fd_v4l, VIDIOC_REQBUFS, &buf_req) < 0)
        {
                printf("request buffers failed\n");
                return TFAIL;
        }
        g_num_buffers = buf_req.count;
        printf("v4l2_output test: Allocated %d buffers\n", buf_req.count);

        return TPASS;
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
                else if (strcmp(argv[i], "-u") == 0) {
			g_mem_type = V4L2_MEMORY_USERPTR;
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
                        g_dev = argv[++i];
                }
                else if (strcmp(argv[i], "-fg") == 0) {
                        g_overlay = 1;
                }
                else if (strcmp(argv[i], "-r") == 0) {
                        g_rotate = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-vf") == 0) {
                        g_vflip = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-hf") == 0) {
                        g_hflip = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-v") == 0) {
			g_vdi_enable = 1;
                        g_vdi_motion = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-fr") == 0) {
                        g_frame_period = atoi(argv[++i]);
                        g_frame_period = 1000000L / g_frame_period;
                }
                else if (strcmp(argv[i], "-l") == 0) {
                        g_loop_count = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-cr") == 0) {
                        g_icrop_w = atoi(argv[++i]);
                        g_icrop_h = atoi(argv[++i]);
                        g_icrop_left = atoi(argv[++i]);
                        g_icrop_top = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-f") == 0) {
                        i++;
                        g_in_fmt = v4l2_fourcc(argv[i][0], argv[i][1],argv[i][2],argv[i][3]);

                        if ( (g_in_fmt != V4L2_PIX_FMT_BGR24) &&
                             (g_in_fmt != V4L2_PIX_FMT_BGR32) &&
                             (g_in_fmt != V4L2_PIX_FMT_RGB565) &&
                             (g_in_fmt != 'PMBW') &&
                             (g_in_fmt != V4L2_PIX_FMT_UYVY) &&
			     (g_in_fmt != V4L2_PIX_FMT_YUYV) &&
			     (g_in_fmt != V4L2_PIX_FMT_YUV422P) &&
			     (g_in_fmt != IPU_PIX_FMT_YUV444P) &&
                             (g_in_fmt != V4L2_PIX_FMT_YUV420) &&
                             (g_in_fmt != V4L2_PIX_FMT_YVU420) &&
                             (g_in_fmt != IPU_PIX_FMT_TILED_NV12) &&
                             (g_in_fmt != IPU_PIX_FMT_TILED_NV12F) &&
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

void memfree(int buf_size, int buf_cnt)
{
	int i;
        unsigned int page_size;

	page_size = getpagesize();
	buf_size = (buf_size + page_size - 1) & ~(page_size - 1);

	for (i = 0; i < buf_cnt; i++) {
		if (buffers[i].start)
			munmap(buffers[i].start, buf_size);
		if (buffers[i].offset)
			ioctl(fd_ipu, IPU_FREE, &buffers[i].offset);
	}
	close(fd_ipu);
}

int memalloc(int buf_size, int buf_cnt)
{
	int i, ret = TPASS;
        unsigned int page_size;

	fd_ipu = open("/dev/mxc_ipu", O_RDWR, 0);
	if (fd_ipu < 0) {
		printf("open ipu dev fail\n");
		return TFAIL;
	}

	for (i = 0; i < buf_cnt; i++) {
		page_size = getpagesize();
		buf_size = (buf_size + page_size - 1) & ~(page_size - 1);
		buffers[i].length = buffers[i].offset = buf_size;
		ret = ioctl(fd_ipu, IPU_ALLOC, &buffers[i].offset);
		if (ret < 0) {
			printf("ioctl IPU_ALLOC fail\n");
			ret = TFAIL;
			goto err;
		}
		buffers[i].start = mmap(0, buf_size, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd_ipu, buffers[i].offset);
		if (!buffers[i].start) {
			printf("mmap fail\n");
			ret = TFAIL;
			goto err;
		}
		printf("USRP: alloc bufs offset 0x%x size %d\n", buffers[i].offset, buf_size);
	}

	return ret;
err:
	memfree(buf_size, buf_cnt);
	return ret;
}

int
main(int argc, char **argv)
{
        struct v4l2_control ctrl;
        struct v4l2_format fmt;
        struct v4l2_framebuffer fb;
        struct v4l2_cropcap cropcap;
	struct v4l2_capability cap;
	struct v4l2_fmtdesc fmtdesc;
        struct v4l2_crop crop;
	struct v4l2_rect icrop;
        FILE * fd_in;
        int retval = TPASS;

        if (process_cmdline(argc, argv) < 0) {
                printf("MXC Video4Linux Output Device Test\n\n" \
                       "Syntax: mxc_v4l2_output.out\n -d <output display dev>\n" \
                       "-iw <input width> -ih <input height>\n" \
                       "[-u if defined, means use userp, otherwise mmap]\n" \
                       "[-cr <input crop w, h, x, y>]\n [-f <WBMP|fourcc code>]" \
                       "[-ow <display width> -oh <display height>]\n" \
                       "[-ot <display top> -ol <display left>]\n" \
                       "[-r <rotate mode 0, 90, 180, 270>]\n [-vf <vflip 0,1>] [-hf <hflip 0,1>]\n" \
                       "[-v <deinterlacing motion level 0 - low, 1 - medium, 2 - high>]\n" \
                       "[-fr <frame rate (fps)>] [-l <loop count>]\n" \
                       "<input YUV file>\n\n");
                retval = TFAIL;
                goto err0;
        }

        if ((fd_v4l = open(g_dev, O_RDWR, 0)) < 0)
        {
                printf("Unable to open %s\n", g_dev);
                retval = TFAIL;
                goto err0;
        }

	if (!ioctl(fd_v4l, VIDIOC_QUERYCAP, &cap)) {
		printf("driver=%s, card=%s, bus=%s, "
				"version=0x%08x, "
				"capabilities=0x%08x\n",
				cap.driver, cap.card, cap.bus_info,
				cap.version,
				cap.capabilities);
	}

	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	while (!ioctl(fd_v4l, VIDIOC_ENUM_FMT, &fmtdesc)) {
		printf("fmt %s: fourcc = 0x%08x\n",
				fmtdesc.description,
				fmtdesc.pixelformat);
		fmtdesc.index++;
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
        ctrl.id = V4L2_CID_ROTATE;
        ctrl.value = g_rotate;
        if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
        {
                printf("set ctrl rotate failed\n");
                retval = TFAIL;
                goto err1;
        }
        ctrl.id = V4L2_CID_VFLIP;
        ctrl.value = g_vflip;
        if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
        {
                printf("set ctrl vflip failed\n");
                retval = TFAIL;
                goto err1;
        }
        ctrl.id = V4L2_CID_HFLIP;
        ctrl.value = g_hflip;
        if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
        {
                printf("set ctrl hflip failed\n");
                retval = TFAIL;
                goto err1;
        }
	if (g_vdi_enable) {
		ctrl.id = V4L2_CID_MXC_MOTION;
		ctrl.value = g_vdi_motion;
		if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
		{
			printf("set ctrl motion failed\n");
			retval = TFAIL;
			goto err1;
		}
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
	if (g_vdi_enable)
		fmt.fmt.pix.field = V4L2_FIELD_INTERLACED_TB;
	if (g_icrop_w && g_icrop_h) {
		icrop.left = g_icrop_left;
		icrop.top = g_icrop_top;
		icrop.width = g_icrop_w;
		icrop.height = g_icrop_h;
		fmt.fmt.pix.priv = (unsigned int)&icrop;
        } else
		fmt.fmt.pix.priv = 0;
        retval = mxc_v4l_output_setup(&fmt);
	if (retval < 0)
		goto err1;

        g_frame_size = fmt.fmt.pix.sizeimage;

	if (g_mem_type == V4L2_MEMORY_USERPTR)
		if (memalloc(g_frame_size, g_num_buffers) < 0)
			goto err1;

        fd_in = fopen(argv[argc-1], "rb");
        if (fd_in)
                fseek(fd_in, 0, SEEK_SET);

        retval = mxc_v4l_output_test(fd_in);

	if (g_mem_type == V4L2_MEMORY_USERPTR)
		memfree(g_frame_size, g_num_buffers);

        if (fd_in)
                fclose(fd_in);
err1:
        close(fd_v4l);
err0:
        return retval;
}

