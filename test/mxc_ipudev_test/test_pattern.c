/*
 * Copyright 2009 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 */

/*
 * The code contained herein is licensed under the GNU Lesser General
 * Public License.  You may obtain a copy of the GNU Lesser General
 * Public License Version 2.1 or later at the following locations:
 *
 * http://www.opensource.org/licenses/lgpl-license.html
 * http://www.gnu.org/copyleft/lgpl.html
 */

/*!
 * @file test_pattern.c
 *
 * @brief IPU device lib test pattern implementation
 *
 * @ingroup IPU
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/mxcfb.h>
#include "mxc_ipudev_test.h"

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
#define BUF_CNT		5

extern int ctrl_c_rev;

static int get_system_rev(unsigned int * system_rev)
{
        FILE *fp;
        char buf[1024];
        int nread;
        char *tmp, *rev;
        int ret = -1;

        fp = fopen("/proc/cpuinfo", "r");
        if (fp == NULL) {
                printf("Open /proc/cpuinfo failed!\n");
                return ret;
        }

        nread = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if ((nread == 0) || (nread == sizeof(buf))) {
                fclose(fp);
                return ret;
        }

        buf[nread] = '\0';

        tmp = strstr(buf, "Revision");
        if (tmp != NULL) {
                rev = index(tmp, ':');
                if (rev != NULL) {
                        rev++;
                        *system_rev = strtoul(rev, NULL, 16);
                        ret = 0;
                }
        }

        return ret;
}

void gen_fill_pattern(char * buf, int in_width, int in_height)
{
	int y_size = in_width * in_height;
	int h_step = in_height / 16;
	int w_step = in_width / 16;
	int h, w;
	uint32_t y_color = 0;
	int32_t u_color = 0;
	int32_t v_color = 0;
	uint32_t rgb = 0;
	static int32_t alpha = 0;
	static int inc_alpha = 1;

	for (h = 0; h < in_height; h++) {
		int32_t rgb_temp = rgb;

		for (w = 0; w < in_width; w++) {
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
			buf[(h*in_width) + w] = y_color;
			if (!(h & 0x1) && !(w & 0x1)) {
				buf[y_size + (((h*in_width)/4) + (w/2)) ] = u_color;
				buf[y_size + y_size/4 + (((h*in_width)/4) + (w/2))] = v_color;
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

int foreground_fb(void)
{
	int fd_fb;
	struct fb_fix_screeninfo fb_fix;

	fd_fb = open("/dev/fb2", O_RDWR, 0);
	ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix);
	if (strcmp(fb_fix.id, "DISP3 FG") == 0) {
		close(fd_fb);
		return 2;
	}

	fd_fb = open("/dev/fb1", O_RDWR, 0);
	ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix);
	if (strcmp(fb_fix.id, "DISP3 FG") == 0) {
		close(fd_fb);
		return 1;
	}

	return 0;
}

int color_bar(int two_output, ipu_test_handle_t * test_handle)
{
	int ret = 0, fd_fb = 0, size = 0, i, done_cnt = 0;
	void * buf[BUF_CNT] = {0};
	int paddr[BUF_CNT] = {0};
	struct fb_var_screeninfo fb_var;
	unsigned int system_rev = 0, ipu_version;

	get_system_rev(&system_rev);
	if (((system_rev & 0xff000) == 0x37000) ||
		(((system_rev & 0xff000) == 0x51000)))
		ipu_version = 3;
	else
		ipu_version = 1;

	if ((ipu_version == 1) && two_output) {
		printf("ipuv1 can not support dispaly 2 output together!\n");
		printf("because ipuv1 ENC channel can not be linked to disp channel!\n");
		ret = -1;
		goto done;
	}

	if ((fd_fb = open("/dev/fb0", O_RDWR, 0)) < 0) {
		printf("Unable to open /dev/fb0\n");
		ret = -1;
		goto done;
	}

	if ( ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var) < 0) {
		printf("Get FB var info failed!\n");
		ret = -1;
		goto done;
	}

	/* use I420 input format as fix*/
	test_handle->mode = OP_STREAM_MODE;
	test_handle->fcount = 300;
	test_handle->input.width = 320;
	test_handle->input.height = 240;
	test_handle->input.fmt = v4l2_fourcc('I', '4', '2', '0');
	if (fb_var.bits_per_pixel == 24)
		test_handle->output0.fmt = v4l2_fourcc('B', 'G', 'R', '3');
	else
		test_handle->output0.fmt = v4l2_fourcc('R', 'G', 'B', 'P');
	test_handle->output0.show_to_fb = 1;
	test_handle->output0.fb_disp.fb_num = 0;
	/* ipuv1 only support display on PP & VF task mode */
	if (ipu_version == 1)
		test_handle->mode |= TASK_PP_MODE;
	test_handle->output0.rot = 3;
	if (two_output) {
		test_handle->output0.width = fb_var.xres;
		test_handle->output0.height = fb_var.yres/3;
		test_handle->output0.fb_disp.pos.x = 0;
		test_handle->output0.fb_disp.pos.y = 0;
		test_handle->output1.width = fb_var.xres;
		test_handle->output1.height = fb_var.yres/3;
		if (fb_var.bits_per_pixel == 24)
			test_handle->output1.fmt = v4l2_fourcc('B', 'G', 'R', '3');
		else
			test_handle->output1.fmt = v4l2_fourcc('R', 'G', 'B', 'P');
		test_handle->output1.show_to_fb = 1;
		test_handle->output1.fb_disp.fb_num = foreground_fb();
		test_handle->output1.rot = 3;
		test_handle->output1.fb_disp.pos.x = 0;
		test_handle->output1.fb_disp.pos.y = fb_var.yres/2;
	} else {
		test_handle->output0.width = fb_var.xres;
		test_handle->output0.height = fb_var.yres;
	}

	/*allocate dma buffers from fb dev*/
	size = test_handle->input.width * test_handle->input.height * 3/2;
	for (i=0;i<BUF_CNT;i++) {
		/*alloc mem from DMA zone*/
		/*input as request mem size */
		paddr[i] = size;
		if ( ioctl(fd_fb, FBIO_ALLOC, &(paddr[i])) < 0) {
			printf("Unable alloc mem from /dev/fb0\n");
			close(fd_fb);
			if ((fd_fb = open("/dev/fb1", O_RDWR, 0)) < 0) {
				printf("Unable to open /dev/fb1\n");
				if ((fd_fb = open("/dev/fb2", O_RDWR, 0)) < 0) {
					printf("Unable to open /dev/fb2\n");
					ret = -1;
					goto done;
				} else if ( ioctl(fd_fb, FBIO_ALLOC, &(paddr[i])) < 0) {
					printf("Unable alloc mem from /dev/fb2\n");
					ret = -1;
					goto done;
				}
			} else if ( ioctl(fd_fb, FBIO_ALLOC, &(paddr[i])) < 0) {
				printf("Unable alloc mem from /dev/fb1\n");
				close(fd_fb);
				if ((fd_fb = open("/dev/fb2", O_RDWR, 0)) < 0) {
					printf("Unable to open /dev/fb2\n");
					ret = -1;
					goto done;
				} else if ( ioctl(fd_fb, FBIO_ALLOC, &(paddr[i])) < 0) {
					printf("Unable alloc mem from /dev/fb2\n");
					ret = -1;
					goto done;
				}
			}
		}

		buf[i] = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
				fd_fb, paddr[i]);
		if (buf[i] == MAP_FAILED) {
			printf("mmap failed!\n");
			ret = -1;
			goto done;
		}
	}

	/* we are using stream mode and we set dma addr by ourselves*/
	test_handle->input.paddr[0] = paddr[0];
	test_handle->input.paddr[1] = paddr[1];
	gen_fill_pattern(buf[0], test_handle->input.width, test_handle->input.height);
	gen_fill_pattern(buf[1], test_handle->input.width, test_handle->input.height);
	done_cnt = i = 1;

	if (two_output)
		ret = mxc_ipu_lib_task_init(&(test_handle->input), &(test_handle->output0),
				&(test_handle->output1), test_handle->mode, test_handle->ipu_handle);
	else
		ret = mxc_ipu_lib_task_init(&(test_handle->input), &(test_handle->output0),
				NULL, test_handle->mode, test_handle->ipu_handle);
	if (ret < 0) {
		printf("mxc_ipu_lib_task_init failed!\n");
		goto done;
	}

	while((done_cnt < test_handle->fcount) && (ctrl_c_rev == 0)) {
		if (mxc_ipu_lib_task_buf_update(test_handle->ipu_handle, paddr[i], NULL, NULL) < 0)
			break;
		i++;
		if (i == BUF_CNT)
			i = 0;
		done_cnt++;
		gen_fill_pattern(buf[i], test_handle->input.width, test_handle->input.height);
	}

	mxc_ipu_lib_task_uninit(test_handle->ipu_handle);

done:
	for (i=0;i<BUF_CNT;i++) {
		if (buf[i])
			munmap(buf[i], size);
		if (paddr[i])
			ioctl(fd_fb, FBIO_FREE, &(paddr[i]));
	}
	if (fd_fb)
		close(fd_fb);

	return ret;
}

int update_block_pos(int *x, int *y, int angle_start, int block_width,
	int limit_w, int limit_h, int fd_fb)
{
	static int angle = 0;
	static double steps = 0.0;
	struct mxcfb_pos pos;
	int need_change = 0;
	double ra;
	double nx, ny;

	if (angle == 0)
		angle = angle_start;
	steps += 0.1;
	ra = ((double)angle)*3.1415926/180.0;
	nx = *x + steps*cos(ra);
	ny = *y + steps*sin(ra);
	if (nx < 0) {
		if (ny < *y) /* x+, y-*/
			angle = 360 - angle_start;
		else /* x+, y+ */
			angle = angle_start;
		need_change = 1;
	} else if (nx > (limit_w - block_width)) {
		if (ny < *y) /* x-, y-*/
			angle = 180 + angle_start;
		else /* x-, y+ */
			angle = 180 - angle_start;
		need_change = 1;
	} else if (ny < 0) {
		if (nx < *x) /* x-, y+*/
			angle = 180 - angle_start;
		else /* x+, y+ */
			angle = angle_start;
		need_change = 1;
	} else if (ny > (limit_h - block_width)) {
		if (nx < *x) /* x-, y-*/
			angle = 180 + angle_start;
		else /* x+, y- */
			angle = 360 - angle_start;
		need_change = 1;
	}

	if (need_change) {
		steps = 0.0;
		ra = ((double)angle)*3.1415926/180.0;
		nx = *x;
		ny = *y;
		//printf("change angle to %d\n", angle);
		//printf("pos %d, %d\n", *x, *y);
	}

	pos.x = nx;
	pos.y = ny;
	ioctl(fd_fb, MXCFB_SET_OVERLAY_POS, &pos);
	*x = nx;
	*y = ny;

	return need_change;
}

static void my_msleep(int ms)
{
      struct timeval tv;

      tv.tv_sec = ms/1000;
      tv.tv_usec = (ms % 1000) * 1000;
      select(0, NULL, NULL, NULL, &tv);
}

/*
 * This call-back function provide one method to update
 * framebuffer by pan_display.
 */
void hop_block_output_cb(void * arg, int index)
{
	int fd_fb = *((int *)arg);
	struct fb_var_screeninfo fb_var;

	ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var);
	/* for buf index 0, its phyaddr is fb buf1*/
	/* for buf index 1, its phyaddr is fb buf0*/
	if (index == 0)
		fb_var.yoffset = fb_var.yres;
	else
		fb_var.yoffset = 0;
	ioctl(fd_fb, FBIOPAN_DISPLAY, &fb_var);
}

int hop_block(ipu_test_handle_t * test_handle)
{
	int ret = 0, x = 0, y = 0, fd_fb = 0, next_update_idx = 0;
	int lcd_w, lcd_h;
	int blank;
	char random_color;
	int start_angle, screen_size;
	void * buf;
	struct mxcfb_pos pos;
	struct fb_var_screeninfo fb_var;
	struct fb_fix_screeninfo fb_fix;

	/* clear background fb, get the lcd frame info */
	if ((fd_fb = open("/dev/fb0", O_RDWR, 0)) < 0) {
		printf("Unable to open /dev/fb0\n");
		ret = -1;
		goto done;
	}

	if ( ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix) < 0) {
		printf("Get FB fix info failed!\n");
		ret = -1;
		goto done;
	}

	if ( ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var) < 0) {
		printf("Get FB var info failed!\n");
		ret = -1;
		goto done;
	}
	lcd_w = fb_var.xres;
	lcd_h = fb_var.yres;
	screen_size = fb_var.yres_virtual * fb_fix.line_length;

	buf = mmap(NULL, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd_fb, 0);
	if (buf == MAP_FAILED) {
		printf("mmap failed!\n");
		ret = -1;
		goto done;
	}
	memset(buf, 0, screen_size);
	close(fd_fb);

	/* display hop block to overlay */
	if (foreground_fb() == 2) {
		if ((fd_fb = open("/dev/fb2", O_RDWR, 0)) < 0) {
			printf("Unable to open /dev/fb2\n");
			ret = -1;
			goto done;
		}
	} else {
		if ((fd_fb = open("/dev/fb1", O_RDWR, 0)) < 0) {
			printf("Unable to open /dev/fb1\n");
			ret = -1;
			goto done;
		}
	}

	fb_var.xres = test_handle->block_width
			- test_handle->block_width%8;
	fb_var.xres_virtual = fb_var.xres;
	fb_var.yres = test_handle->block_width;
	fb_var.yres_virtual = fb_var.yres * 2;
	if ( ioctl(fd_fb, FBIOPUT_VSCREENINFO, &fb_var) < 0) {
		printf("Set FB var info failed!\n");
		ret = -1;
		goto done;
	}

	if ( ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix) < 0) {
		printf("Get FB fix info failed!\n");
		ret = -1;
		goto done;
	}

	if ( ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var) < 0) {
		printf("Get FB var info failed!\n");
		ret = -1;
		goto done;
	}
	blank = FB_BLANK_UNBLANK;
	if ( ioctl(fd_fb, FBIOBLANK, blank) < 0) {
		printf("UNBLANK FB failed!\n");
		ret = -1;
		goto done;
	}

	test_handle->mode = OP_STREAM_MODE;
	test_handle->input.width = 400;
	test_handle->input.height = 400;
	test_handle->input.fmt = v4l2_fourcc('R', 'G', 'B', 'P');
	test_handle->output0.width = test_handle->block_width
					- test_handle->block_width%8;
	test_handle->output0.height = test_handle->block_width;
	if (fb_var.bits_per_pixel == 24)
		test_handle->output0.fmt = v4l2_fourcc('B', 'G', 'R', '3');
	else
		test_handle->output0.fmt = v4l2_fourcc('R', 'G', 'B', 'P');
	test_handle->output0.show_to_fb = 0;
	screen_size = fb_var.yres * fb_fix.line_length;
	test_handle->output0.paddr[0] = fb_fix.smem_start + screen_size;
	test_handle->output0.paddr[1] = fb_fix.smem_start;

	ret = mxc_ipu_lib_task_init(&(test_handle->input), &(test_handle->output0),
			NULL, test_handle->mode, test_handle->ipu_handle);
	if (ret < 0) {
		printf("mxc_ipu_lib_task_init failed!\n");
		goto done;
	}

	srand((unsigned int)time(0));
	random_color = (char)(rand()%255);
	/* for stream mode, fill two input frame to prepare */
	memset(test_handle->ipu_handle->inbuf_start[0], random_color, test_handle->ipu_handle->ifr_size);
	memset(test_handle->ipu_handle->inbuf_start[1], random_color, test_handle->ipu_handle->ifr_size);
	start_angle = rand()%90;
	if (start_angle == 90) start_angle = 89;
	if (start_angle == 0) start_angle = 1;
	printf("Start angle is %d\n", start_angle);

	/* start first frame */
	if((next_update_idx = mxc_ipu_lib_task_buf_update(test_handle->ipu_handle, 0, hop_block_output_cb, &fd_fb)) < 0)
		goto err;

	while(ctrl_c_rev == 0) {
		my_msleep(100);
		/* update frame if only hop block hit the LCD frame */
		if(update_block_pos(&x, &y, start_angle, test_handle->block_width, lcd_w, lcd_h, fd_fb)) {
			random_color = (char)(rand()%255);
			memset(test_handle->ipu_handle->inbuf_start[next_update_idx], random_color,
					test_handle->ipu_handle->ifr_size);
			if((next_update_idx = mxc_ipu_lib_task_buf_update(test_handle->ipu_handle, 0, hop_block_output_cb, &fd_fb)) < 0)
				break;
		}
	}

	/* ipu need reset position to 0,0 */
	pos.x = 0;
	pos.y = 0;
	ioctl(fd_fb, MXCFB_SET_OVERLAY_POS, &pos);

	blank = FB_BLANK_POWERDOWN;
	if ( ioctl(fd_fb, FBIOBLANK, blank) < 0) {
		printf("POWERDOWN FB failed!\n");
		ret = -1;
		goto done;
	}
err:
	mxc_ipu_lib_task_uninit(test_handle->ipu_handle);

done:
	if (fd_fb)
		close(fd_fb);
	return ret;
}

void * thread_func(void *arg)
{
	ipu_test_handle_t test_handle;
	ipu_lib_handle_t ipu_handle;

	memset(&ipu_handle, 0, sizeof(ipu_lib_handle_t));
	memset(&test_handle, 0, sizeof(ipu_test_handle_t));
	test_handle.ipu_handle = &ipu_handle;

	color_bar(0, &test_handle);

	return NULL;
}

int run_test_pattern(int pattern, ipu_test_handle_t * test_handle)
{
	if (pattern == 1)
		return color_bar(0, test_handle);
	if (pattern == 2)
		return color_bar(1, test_handle);
	if (pattern == 3)
		return hop_block(test_handle);
	if (pattern == 4) {
		pthread_t thread1;

		pthread_create(&thread1, NULL, thread_func, NULL);

		return hop_block(test_handle);
	}

	printf("No such test pattern %d\n", pattern);
	return -1;
}
