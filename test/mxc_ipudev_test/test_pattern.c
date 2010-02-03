/*
 * Copyright 2009-2010 Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/mxcfb.h>
#include "mxc_ipudev_test.h"
#include "ScreenLayer.h"

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

void gen_fill_alpha_in_separate_buffer(void * alpha_buf, int alpha_size,
				       char alpha)
{
	memset(alpha_buf, alpha, alpha_size);
}

void fill_alpha_buffer(char *alpha_buf, int left, int top,
		       int right, int bottom, int disp_w, char alpha_val)
{
	char *pPointAlphaValue;
	int x, y;

	for (y = top; y < bottom; y++) {
		for (x = left; x < right; x++) {
			pPointAlphaValue = (char *)(alpha_buf +
					    disp_w * y + x);
			*pPointAlphaValue = alpha_val;
		}
	}
}

void gen_fill_alpha_in_pixel(void * buf, unsigned int pixel_format,
			     int buf_size, char alpha)
{
	int i;
	char* p_alpha;

	if (pixel_format == v4l2_fourcc('R', 'G', 'B', 'A') ||
	    pixel_format == v4l2_fourcc('B', 'G', 'R', 'A')) {
		for (i = 0; i < buf_size; i++) {
			if (i % 4 == 3) {
				p_alpha = (char *)(buf + i);
				*p_alpha = alpha;
			}
		}
	} else if (pixel_format == v4l2_fourcc('A', 'B', 'G', 'R')) {
		for (i = 0; i < buf_size; i++) {
			if (i % 4 == 0) {
				p_alpha = (char *)(buf + i);
				*p_alpha = alpha;
			}
		}
	} else {
		printf("Unsupported pixel format with alpha value!\n");
	}
}

void gen_fill_alpha_in_pixel_for_point(void * buf, unsigned int pixel_format,
			     int sl_width, int x, int y, char alpha)
{
	unsigned char* p_alpha;

	if (pixel_format == v4l2_fourcc('R', 'G', 'B', 'A') ||
	    pixel_format == v4l2_fourcc('B', 'G', 'R', 'A')) {
		p_alpha = (u8 *)(buf + 4*sl_width*y + 4*x + 3);
	} else if (pixel_format == v4l2_fourcc('A', 'B', 'G', 'R')) {
		p_alpha = (u8 *)(buf + 4*sl_width*y + 4*x);
	} else {
		printf("Unsupported pixel format with alpha value!\n");
		return;
	}
	*p_alpha = alpha;
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

int dc_fb(void)
{
	int fd_fb;
	struct fb_fix_screeninfo fb_fix;

	fd_fb = open("/dev/fb0", O_RDWR, 0);
	ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix);
	if (strcmp(fb_fix.id, "DISP3 BG - DI1") == 0) {
		close(fd_fb);
		return 0;
	}

	fd_fb = open("/dev/fb1", O_RDWR, 0);
	ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix);
	if (strcmp(fb_fix.id, "DISP3 BG - DI1") == 0) {
		close(fd_fb);
		return 1;
	}

	return -1;
}

int fd_fb_alloc = 0;

int dma_memory_alloc(int size, int cnt, dma_addr_t paddr[], void * vaddr[])
{
	int i, ret = 0;

	if ((fd_fb_alloc = open("/dev/fb0", O_RDWR, 0)) < 0) {
		printf("Unable to open /dev/fb0\n");
		ret = -1;
		goto done;
	}

	for (i=0;i<cnt;i++) {
		/*alloc mem from DMA zone*/
		/*input as request mem size */
		paddr[i] = size;
		if ( ioctl(fd_fb_alloc, FBIO_ALLOC, &(paddr[i])) < 0) {
			printf("Unable alloc mem from /dev/fb0\n");
			close(fd_fb_alloc);
			if ((fd_fb_alloc = open("/dev/fb1", O_RDWR, 0)) < 0) {
				printf("Unable to open /dev/fb1\n");
				if ((fd_fb_alloc = open("/dev/fb2", O_RDWR, 0)) < 0) {
					printf("Unable to open /dev/fb2\n");
					ret = -1;
					goto done;
				} else if ( ioctl(fd_fb_alloc, FBIO_ALLOC, &(paddr[i])) < 0) {
					printf("Unable alloc mem from /dev/fb2\n");
					ret = -1;
					goto done;
				}
			} else if ( ioctl(fd_fb_alloc, FBIO_ALLOC, &(paddr[i])) < 0) {
				printf("Unable alloc mem from /dev/fb1\n");
				close(fd_fb_alloc);
				if ((fd_fb_alloc = open("/dev/fb2", O_RDWR, 0)) < 0) {
					printf("Unable to open /dev/fb2\n");
					ret = -1;
					goto done;
				} else if ( ioctl(fd_fb_alloc, FBIO_ALLOC, &(paddr[i])) < 0) {
					printf("Unable alloc mem from /dev/fb2\n");
					ret = -1;
					goto done;
				}
			}
		}

		vaddr[i] = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
				fd_fb_alloc, paddr[i]);
		if (vaddr[i] == MAP_FAILED) {
			printf("mmap failed!\n");
			ret = -1;
			goto done;
		}
	}
done:
	return ret;
}

void dma_memory_free(int size, int cnt, dma_addr_t paddr[], void * vaddr[])
{
	int i;

	for (i=0;i<cnt;i++) {
		if (vaddr[i])
			munmap(vaddr[i], size);
		if (paddr[i])
			ioctl(fd_fb_alloc, FBIO_FREE, &(paddr[i]));
	}
}

int copy_test(ipu_test_handle_t * test_handle)
{
	int ret = 0, fd_fb = 0, screen_size;
	struct fb_var_screeninfo fb_var;
	struct fb_fix_screeninfo fb_fix;
	void * fake_fb[1];
	int fake_fb_paddr[1], done_cnt = 0;

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
	if ( ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix) < 0) {
		printf("Get FB fix info failed!\n");
		ret = -1;
		goto done;
	}
	screen_size = fb_var.yres * fb_fix.line_length;
	ret = dma_memory_alloc(screen_size, 1, fake_fb_paddr, fake_fb);
	if ( ret < 0) {
		printf("dma_memory_alloc failed\n");
		goto done;
	}

	test_handle->mode = OP_NORMAL_MODE | TASK_PP_MODE;
	test_handle->fcount = 10;
	test_handle->input.width = fb_var.xres;
	test_handle->input.height = fb_var.yres;
	test_handle->output0.width = fb_var.xres;
	test_handle->output0.height = fb_var.yres;
	if (fb_var.bits_per_pixel == 24) {
		test_handle->output0.fmt = v4l2_fourcc('B', 'G', 'R', '3');
		test_handle->input.fmt = v4l2_fourcc('B', 'G', 'R', '3');
	} else {
		test_handle->output0.fmt = v4l2_fourcc('R', 'G', 'B', 'P');
		test_handle->input.fmt = v4l2_fourcc('R', 'G', 'B', 'P');
	}
	test_handle->input.user_def_paddr[0] = fake_fb_paddr[0];
	test_handle->output0.user_def_paddr[0] = fb_fix.smem_start;

	ret = mxc_ipu_lib_task_init(&(test_handle->input), NULL, &(test_handle->output0),
			NULL, test_handle->mode, test_handle->ipu_handle);
	if (ret < 0) {
		printf("mxc_ipu_lib_task_init failed!\n");
		goto err;
	}

	while((done_cnt < test_handle->fcount) && (ctrl_c_rev == 0)) {
		static int j = 0;
		if ((j % 3) == 0)
			memset(fake_fb[0], 0, screen_size);
		if ((j % 3) == 1)
			memset(fake_fb[0], 0x80, screen_size);
		if ((j % 3) == 2)
			memset(fake_fb[0], 0xff, screen_size);
		j++;
		if (mxc_ipu_lib_task_buf_update(test_handle->ipu_handle, 0, 0, 0, NULL, NULL) < 0)
			break;
		done_cnt++;
		sleep(1);
	}

	mxc_ipu_lib_task_uninit(test_handle->ipu_handle);

err:
	dma_memory_free(screen_size, 1, fake_fb_paddr, fake_fb);
done:
	return ret;
}

int color_bar(int two_output, int overlay, ipu_test_handle_t * test_handle)
{
	int ret = 0, fd_fb = 0, size = 0, i, k = 0, done_cnt = 0, fcount = 0;
	void * buf[BUF_CNT] = {0}, * fb[2];
	void * ov_fake_fb = 0, * ov_alpha_fake_fb = 0;
	int ov_fake_fb_paddr = 0, ov_alpha_fake_fb_paddr = 0;
	int paddr[BUF_CNT] = {0};
	struct fb_var_screeninfo fb_var;
	struct fb_fix_screeninfo fb_fix;
	struct mxcfb_gbl_alpha g_alpha;
	unsigned int system_rev = 0, ipu_version;
	ipu_lib_overlay_param_t ov;
	int screen_size, ov_fake_fb_size = 0, ov_alpha_fake_fb_size = 0;

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
	if ((ipu_version == 1) && overlay) {
		printf("currently ipuv1 would not support overlay!\n");
		ret = -1;
		goto done;
	}

	if ((fd_fb = open("/dev/fb0", O_RDWR, 0)) < 0) {
		printf("Unable to open /dev/fb0\n");
		ret = -1;
		goto done;
	}

	g_alpha.alpha = 128;
	g_alpha.enable = 1;
	if (ioctl(fd_fb, MXCFB_SET_GBL_ALPHA, &g_alpha) < 0) {
		printf("Set global alpha failed\n");
		ret = -1;
		goto done;
	}

	if ( ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var) < 0) {
		printf("Get FB var info failed!\n");
		ret = -1;
		goto done;
	}
	if ( ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix) < 0) {
		printf("Get FB fix info failed!\n");
		ret = -1;
		goto done;
	}

	if(fb_var.yres_virtual != 2*fb_var.yres)
	{
		fb_var.yres_virtual = 2*fb_var.yres;
		if ( ioctl(fd_fb, FBIOPUT_VSCREENINFO, &fb_var) < 0) {
			printf("Get FB var info failed!\n");
			ret = -1;
			goto done;
		}
	}

	screen_size = fb_var.yres * fb_fix.line_length;
	fb[0] = mmap(NULL, 2 * screen_size, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd_fb, 0);
	if (fb[0] == MAP_FAILED) {
		printf("fb buf0 mmap failed, errno %d!\n", errno);
		ret = -1;
		goto done;
	}
	fb[1] = (void *)((char *)fb[0] + screen_size);

	/* use I420 input format as fix*/
	test_handle->mode = OP_STREAM_MODE;
	test_handle->fcount = fcount = 511;
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
		/* two output case -- show to BG+FG */
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
	} else if (overlay){
		/* overlay case -- fake fb+overlay show to fb0*/
		ov.width = fb_var.xres;
		ov.height = fb_var.yres;
		if (fb_var.bits_per_pixel == 24)
			ov.fmt = v4l2_fourcc('B', 'G', 'R', '3');
		else
			ov.fmt = v4l2_fourcc('R', 'G', 'B', 'P');

		if (overlay == IC_LOC_SEP_ALP_OV) {
			ov.ov_crop_win.pos.x = fb_var.xres/4;
			ov.ov_crop_win.pos.y = fb_var.yres/4;
			ov.ov_crop_win.win_w = fb_var.xres/2;
			ov.ov_crop_win.win_h = fb_var.yres/2;
			ov.local_alpha_en = 1;
			ov.global_alpha_en = 0;
			ov_fake_fb_size = screen_size;
			ov_alpha_fake_fb_size = ov.width * ov.height;

			test_handle->output0.width = fb_var.xres/2;
			test_handle->output0.height = fb_var.yres/2;
			test_handle->output0.fb_disp.pos.x = fb_var.xres/4;
			test_handle->output0.fb_disp.pos.y = fb_var.yres/4;
		} else if (overlay == IC_GLB_ALP_OV) {
			ov.ov_crop_win.pos.x = fb_var.xres/4;
			ov.ov_crop_win.pos.y = fb_var.yres/4;
			ov.ov_crop_win.win_w = fb_var.xres/2;
			ov.ov_crop_win.win_h = fb_var.yres/2;
			ov.local_alpha_en = 0;
			ov.global_alpha_en = 1;
			ov_fake_fb_size = screen_size;

			test_handle->output0.width = fb_var.xres/2;
			test_handle->output0.height = fb_var.yres/2;
			test_handle->output0.fb_disp.pos.x = fb_var.xres/4;
			test_handle->output0.fb_disp.pos.y = fb_var.yres/4;
		} else if (overlay == IC_LOC_PIX_ALP_OV) {
			ov.fmt = v4l2_fourcc('R', 'G', 'B', 'A');
			ov.ov_crop_win.pos.x = 0;
			ov.ov_crop_win.pos.y = 0;
			ov.ov_crop_win.win_w = fb_var.xres;
			ov.ov_crop_win.win_h = fb_var.yres;
			ov.global_alpha_en = 0;
			ov.local_alpha_en = 0;
			ov_fake_fb_size = ov.width * ov.height * 4;

			/*
			 * RGB32A is not the pixel format of FB0(RGB565),
			 * so we cannot do memory copy from graphic plane to
			 * FB0. We choose to set graphic plane and video plane
			 * to be the same resolution with FB0.
			 */
			test_handle->output0.width = fb_var.xres;
			test_handle->output0.height = fb_var.yres;
			test_handle->output0.fb_disp.pos.x = 0;
			test_handle->output0.fb_disp.pos.y = 0;
		}
		ov.key_color_en = 0;
		ov.alpha = 0;
		ov.key_color = 0x808080;
	} else {
		/* one output case -- full screen */
		test_handle->output0.width = fb_var.xres;
		test_handle->output0.height = fb_var.yres;
	}

	/*allocate dma buffers from fb dev*/
	size = test_handle->input.width * test_handle->input.height * 3/2;
	ret = dma_memory_alloc(size, BUF_CNT, paddr, buf);
	if ( ret < 0) {
		printf("dma_memory_alloc failed\n");
		goto done;
	}

	if (overlay) {
		ov_fake_fb_paddr = ov_fake_fb_size;
		if ( ioctl(fd_fb, FBIO_ALLOC, &(ov_fake_fb_paddr)) < 0) {
			printf("Unable alloc mem from /dev/fb0\n");
			ret = -1;
			goto done;
		}
		ov_fake_fb = mmap(NULL, ov_fake_fb_size, PROT_READ | PROT_WRITE, MAP_SHARED,
				fd_fb, ov_fake_fb_paddr);
		if (ov_fake_fb == MAP_FAILED) {
			printf("mmap failed!\n");
			ret = -1;
			goto done;
		}
		ov.user_def_paddr[0] = ov_fake_fb_paddr;
		ov.user_def_paddr[1] = ov.user_def_paddr[0];

		if (overlay == IC_LOC_SEP_ALP_OV) {
			ov_alpha_fake_fb_paddr = ov_alpha_fake_fb_size;
			if (ioctl(fd_fb, FBIO_ALLOC,
				  &(ov_alpha_fake_fb_paddr)) < 0) {
				printf("Unable alloc mem from /dev/fb0\n");
				ret = -1;
				goto done;
			}
			ov_alpha_fake_fb = mmap(NULL, ov_alpha_fake_fb_size,
						PROT_READ | PROT_WRITE,
						MAP_SHARED,
						fd_fb,
						ov_alpha_fake_fb_paddr);
			if (ov_alpha_fake_fb == MAP_FAILED) {
				printf("mmap failed!\n");
				ret = -1;
				goto done;
			}
			ov.user_def_alpha_paddr[0] = ov_alpha_fake_fb_paddr;
			ov.user_def_alpha_paddr[1] = ov_alpha_fake_fb_paddr;
		}
	}

	/* we are using stream mode and we set dma addr by ourselves*/
	test_handle->input.user_def_paddr[0] = paddr[0];
	test_handle->input.user_def_paddr[1] = paddr[1];
	gen_fill_pattern(buf[0], test_handle->input.width, test_handle->input.height);
	gen_fill_pattern(buf[1], test_handle->input.width, test_handle->input.height);
	done_cnt = i = 1;

	if (two_output)
		ret = mxc_ipu_lib_task_init(&(test_handle->input), NULL, &(test_handle->output0),
				&(test_handle->output1), test_handle->mode, test_handle->ipu_handle);
	else {
		if (overlay)
			ret = mxc_ipu_lib_task_init(&(test_handle->input), &ov, &(test_handle->output0),
				NULL, test_handle->mode, test_handle->ipu_handle);
		else
			ret = mxc_ipu_lib_task_init(&(test_handle->input), NULL, &(test_handle->output0),
				NULL, test_handle->mode, test_handle->ipu_handle);
	}
	if (ret < 0) {
		printf("mxc_ipu_lib_task_init failed!\n");
		goto done;
	}

	while((done_cnt < test_handle->fcount) && (ctrl_c_rev == 0)) {
		if (overlay) {
			if (overlay == IC_GLB_ALP_OV) {
				if (done_cnt % 50 == 0) {
					static int j = 0;
					if ((j % 3) == 0)
						memset(ov_fake_fb, 0,
						       ov_fake_fb_size);
					if ((j % 3) == 1)
						memset(ov_fake_fb, 0x80,
						       ov_fake_fb_size);
					if ((j % 3) == 2)
						memset(ov_fake_fb, 0xff,
						       ov_fake_fb_size);
					j++;
				}
				memcpy(fb[(done_cnt+1)%2], ov_fake_fb,
				       ov_fake_fb_size);
				if (mxc_ipu_lib_task_buf_update(test_handle->ipu_handle, paddr[i], ov_fake_fb_paddr, 0, NULL, NULL) < 0)
					break;
			} else if (overlay == IC_LOC_PIX_ALP_OV) {
				if (done_cnt == 1) {
					/* RGBA32 red */
					/* 2 planes */
					for (k = 0; k < ov_fake_fb_size; k++) {
						if (k % 4 == 0)
							memset(ov_fake_fb + k,
							       0xFF, 1);
						else
							memset(ov_fake_fb + k,
							       0x00, 1);

						if (k % 4 == 3)
							memset(ov_fake_fb + k,
							       0x80, 1);
					}
				} else if (done_cnt == 1*fcount/9) {
					/* video plane */
					for (k = 0; k < ov_fake_fb_size; k++) {
						if (k % 4 == 3)
							memset(ov_fake_fb + k,
							       0x00, 1);
					}
				} else if (done_cnt == 2*fcount/9) {
					/* graphic plane */
					for (k = 0; k < ov_fake_fb_size; k++) {
						if (k % 4 == 3)
							memset(ov_fake_fb + k,
							       0xFF, 1);
					}
				} else if (done_cnt == fcount/3) {
					/* RGBA32 green */
					/* 2 planes */
					for (k = 0; k < ov_fake_fb_size; k++) {
						if (k % 4 == 1)
							memset(ov_fake_fb + k,
							       0xFF, 1);
						else
							memset(ov_fake_fb + k,
							       0x00, 1);

						if (k % 4 == 3)
							memset(ov_fake_fb + k,
							       0x80, 1);
					}
				} else if (done_cnt == 4*fcount/9) {
					/* video plane */
					for (k = 0; k < ov_fake_fb_size; k++) {
						if (k % 4 == 3)
							memset(ov_fake_fb + k,
							       0x00, 1);
					}
				} else if (done_cnt == 5*fcount/9) {
					/* graphic plane */
					for (k = 0; k < ov_fake_fb_size; k++) {
						if (k % 4 == 3)
							memset(ov_fake_fb + k,
							       0xFF, 1);
					}
				} else if (done_cnt == 2*fcount/3) {
					/* RGBA32 blue */
					/* 2 planes */
					for (k = 0; k < ov_fake_fb_size; k++) {
						if (k % 4 == 2)
							memset(ov_fake_fb + k,
							       0xFF, 1);
						else
							memset(ov_fake_fb + k,
							       0x00, 1);

						if (k % 4 == 3)
							memset(ov_fake_fb + k,
							       0x80, 1);
					}
				} else if (done_cnt == 7*fcount/9) {
					/* video plane */
					for (k = 0; k < ov_fake_fb_size; k++) {
						if (k % 4 == 3)
							memset(ov_fake_fb + k,
							       0x00, 1);
					}
				} else if (done_cnt == 8*fcount/9) {
					/* graphic plane */
					for (k = 0; k < ov_fake_fb_size; k++) {
						if (k % 4 == 3)
							memset(ov_fake_fb + k,
							       0xFF, 1);
					}
				}
				if (mxc_ipu_lib_task_buf_update(test_handle->ipu_handle, paddr[i], ov_fake_fb_paddr, 0, NULL, NULL) < 0)
					break;
			} else if (overlay == IC_LOC_SEP_ALP_OV) {
				if (done_cnt == 1) {
					/* RGB565 red */
					for (k = 0; k < ov_fake_fb_size; k++) {
						if (k % 2 == 1)
							memset(ov_fake_fb + k,
							       0xF8, 1);
						else
							memset(ov_fake_fb + k,
							       0x00, 1);
					}
					/* 2 planes */
					memset(ov_alpha_fake_fb, 0x80,
				      	       ov_alpha_fake_fb_size);
				} else if (done_cnt == 1*fcount/9) {
					/* video plane */
					memset(ov_alpha_fake_fb, 0x00,
				      	       ov_alpha_fake_fb_size);
				} else if (done_cnt == 2*fcount/9) {
					/* graphic plane */
					memset(ov_alpha_fake_fb, 0xFF,
				      	       ov_alpha_fake_fb_size);
				} else if (done_cnt == fcount/3) {
					/* RGB565 green */
					for (k = 0; k < ov_fake_fb_size; k++) {
						if (k % 2 == 1)
							memset(ov_fake_fb + k,
							       0x07, 1);
						else
							memset(ov_fake_fb + k,
							       0xE0, 1);
					}
					/* 2 planes */
					memset(ov_alpha_fake_fb, 0x80,
					       ov_alpha_fake_fb_size);
				} else if (done_cnt == 4*fcount/9) {
					/* video plane */
					memset(ov_alpha_fake_fb, 0x00,
				      	       ov_alpha_fake_fb_size);
				} else if (done_cnt == 5*fcount/9) {
					/* graphic plane */
					memset(ov_alpha_fake_fb, 0xFF,
				      	       ov_alpha_fake_fb_size);
				} else if (done_cnt == 2*fcount/3) {
					/* RGB565 blue */
					for (k = 0; k < ov_fake_fb_size; k++) {
						if (k % 2 == 0)
							memset(ov_fake_fb + k,
							       0x1F, 1);
						else
							memset(ov_fake_fb + k,
							       0x00, 1);
					}
					/* 2 planes */
					memset(ov_alpha_fake_fb, 0x80,
					       ov_alpha_fake_fb_size);
				} else if (done_cnt == 7*fcount/9) {
					/* video plane */
					memset(ov_alpha_fake_fb, 0x00,
				      	       ov_alpha_fake_fb_size);
				} else if (done_cnt == 8*fcount/9) {
					/* graphic plane */
					memset(ov_alpha_fake_fb, 0xFF,
				      	       ov_alpha_fake_fb_size);
				}
				memcpy(fb[(done_cnt+1)%2], ov_fake_fb,
				       ov_fake_fb_size);
				if (mxc_ipu_lib_task_buf_update(test_handle->ipu_handle, paddr[i], ov_fake_fb_paddr, ov_alpha_fake_fb_paddr, NULL, NULL) < 0)
					break;
			}
		} else {
			if (mxc_ipu_lib_task_buf_update(test_handle->ipu_handle, paddr[i], 0, 0, NULL, NULL) < 0)
				break;
		}

		i++;
		if (i == BUF_CNT)
			i = 0;
		done_cnt++;
		gen_fill_pattern(buf[i], test_handle->input.width, test_handle->input.height);
		/* make the input framerate < 60Hz*/
		usleep(15000);
	}

	mxc_ipu_lib_task_uninit(test_handle->ipu_handle);

done:
	dma_memory_free(size, BUF_CNT, paddr, buf);
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
	test_handle->output0.user_def_paddr[0] = fb_fix.smem_start + screen_size;
	test_handle->output0.user_def_paddr[1] = fb_fix.smem_start;

	ret = mxc_ipu_lib_task_init(&(test_handle->input), NULL, &(test_handle->output0),
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
	if((next_update_idx = mxc_ipu_lib_task_buf_update(test_handle->ipu_handle, 0, 0, 0, hop_block_output_cb, &fd_fb)) < 0)
		goto err;

	while(ctrl_c_rev == 0) {
		usleep(100000);
		/* update frame if only hop block hit the LCD frame */
		if(update_block_pos(&x, &y, start_angle, test_handle->block_width, lcd_w, lcd_h, fd_fb)) {
			random_color = (char)(rand()%255);
			memset(test_handle->ipu_handle->inbuf_start[next_update_idx], random_color,
					test_handle->ipu_handle->ifr_size);
			if((next_update_idx = mxc_ipu_lib_task_buf_update(test_handle->ipu_handle, 0, 0, 0, hop_block_output_cb, &fd_fb)) < 0)
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

int h_splitted_tv_video_playback(ipu_test_handle_t * test_handle)
{
	int ret = 0, fd_fb = 0, dc_fb_num = 0, screen_size = 0, in_size = 0, i = 0, done_cnt = 0, half_frame_done_cnt = 0, total_fcount = 0;
	void * in_buf[BUF_CNT] = {0};
	int in_paddr[BUF_CNT] = {0};
	char fbdev[] = "/dev/fb0";
	unsigned int system_rev = 0;
	struct fb_var_screeninfo fb_var;
	struct fb_fix_screeninfo fb_fix;

	get_system_rev(&system_rev);
	if (((system_rev & 0xff000) != 0x37000) &&
		(system_rev & 0xff000) != 0x51000) {
		printf("We support to test splitted tv video playback on MX51/MX37 only!\n");
		ret = -1;
		goto done0;
	}

	/* This test assumes the TV uses MEM_DC_SYNC channel */
	dc_fb_num = dc_fb();
	if (dc_fb_num < 0) {
		printf("Can't find the dc fb!\n");
		ret = -1;
		goto done0;
	}

	fbdev[7] = '0' + dc_fb_num;
	if ((fd_fb = open(fbdev, O_RDWR, 0)) < 0) {
		printf("Unable to open the dc fb %s\n", fbdev);
		ret = -1;
		goto done0;
	}

	if ( ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var) < 0) {
		printf("Get FB var info failed!\n");
		ret = -1;
		goto done1;
	}

	fb_var.yres_virtual = 2*fb_var.yres;
	fb_var.nonstd = v4l2_fourcc('U', 'Y', 'V', 'Y');
	if ( ioctl(fd_fb, FBIOPUT_VSCREENINFO, &fb_var) < 0) {
		printf("Get FB var info failed!\n");
		ret = -1;
		goto done1;
	}
	if ( ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix) < 0) {
		printf("Get FB fix info failed!\n");
		ret = -1;
		goto done1;
	}

	screen_size = fb_var.yres * fb_fix.line_length;

	total_fcount = 512;
	test_handle->mode = OP_NORMAL_MODE | TASK_PP_MODE;
	test_handle->input.width = 640;
	test_handle->input.height = 480;
	test_handle->input.fmt = v4l2_fourcc('I', '4', '2', '0');
	test_handle->output0.width = fb_var.xres;
	test_handle->output0.height = fb_var.yres;
	test_handle->output0.fmt = v4l2_fourcc('U', 'Y', 'V', 'Y');
	test_handle->output0.rot = 0;
	test_handle->output0.show_to_fb = 0;

	in_size = test_handle->input.width * test_handle->input.height * 3/2;
	ret = dma_memory_alloc(in_size, BUF_CNT, in_paddr, in_buf);
	if ( ret < 0) {
		printf("dma_memory_alloc input buffer failed\n");
		goto done1;
	}

	gen_fill_pattern(in_buf[0], test_handle->input.width, test_handle->input.height);

	while (done_cnt < total_fcount) {
		test_handle->input.user_def_paddr[0] = in_paddr[i];
		if (half_frame_done_cnt % 2 == 0) {
			test_handle->input.input_crop_win.pos.x = 0;
			test_handle->input.input_crop_win.pos.y = 0;
			test_handle->input.input_crop_win.win_w = test_handle->input.width/2;
			test_handle->input.input_crop_win.win_h = test_handle->input.height;
			test_handle->output0.output_win.win_w = fb_var.xres/2;
			test_handle->output0.output_win.win_h = fb_var.yres;
			test_handle->output0.output_win.pos.x = 0;
			test_handle->output0.output_win.pos.y = 0;
		} else {
			test_handle->input.input_crop_win.pos.x = test_handle->input.width/2;
			test_handle->input.input_crop_win.pos.y = 0;
			test_handle->input.input_crop_win.win_w = test_handle->input.width/2;
			test_handle->input.input_crop_win.win_h = test_handle->input.height;
			test_handle->output0.output_win.win_w = fb_var.xres/2;
			test_handle->output0.output_win.win_h = fb_var.yres;
			test_handle->output0.output_win.pos.x = fb_var.xres/2;
			test_handle->output0.output_win.pos.y = 0;
		}
		if (done_cnt%2)
			test_handle->output0.user_def_paddr[0] = fb_fix.smem_start + screen_size;
		else
			test_handle->output0.user_def_paddr[0] = fb_fix.smem_start;

		ret = mxc_ipu_lib_task_init(&(test_handle->input), NULL,
				&(test_handle->output0), NULL,
				test_handle->mode, test_handle->ipu_handle);
		if (ret < 0) {
			printf("mxc_ipu_lib_task_init failed!\n");
			goto done2;
		}

		if (mxc_ipu_lib_task_buf_update(test_handle->ipu_handle, 0, 0, 0, 0, 0) < 0) {
			printf("mxc_ipu_lib_task_buf_update failed!\n");
			ret = -1;
			mxc_ipu_lib_task_uninit(test_handle->ipu_handle);
			goto done2;
		}

		half_frame_done_cnt++;

		/* One output frame is filled */
		if (half_frame_done_cnt % 2 == 0) {
			i++;
			done_cnt++;
			if (done_cnt % 2)
				fb_var.yoffset = fb_var.yres;
			else
				fb_var.yoffset = 0;
			ioctl(fd_fb, FBIOPAN_DISPLAY, &fb_var);
		}
		if (i == BUF_CNT)
			i = 0;

		if (half_frame_done_cnt % 2 == 0)
			gen_fill_pattern(in_buf[i], test_handle->input.width, test_handle->input.height);

		mxc_ipu_lib_task_uninit(test_handle->ipu_handle);
	}
done2:
	dma_memory_free(in_size, BUF_CNT, in_paddr, in_buf);
done1:
	close(fd_fb);
done0:
	return ret;
}

void * thread_func_color_bar(void *arg)
{
	int ret;
	ipu_test_handle_t test_handle;
	ipu_lib_handle_t ipu_handle;

	memset(&ipu_handle, 0, sizeof(ipu_lib_handle_t));
	memset(&test_handle, 0, sizeof(ipu_test_handle_t));

	test_handle.ipu_handle = &ipu_handle;
	ret = color_bar(0, NO_OV, &test_handle);

	pthread_exit((void*)ret);
}

void * thread_func_hop_block(void *arg)
{
	int ret;

	ret = hop_block((ipu_test_handle_t *)arg);

	pthread_exit((void*)ret);
}

#define PRIMARYFBDEV	"/dev/fb0"
#define OVERLAYFBDEV	"/dev/fb2"
#define	BUFCNT_1ST	1
#define BUFCNT_2ND	3
#define BUFCNT_3TH	5
#define FRM_CNT		511
/* variables for semaphore */
sem_t *         semIDUnit;
const char*     semNameUnit="IPU_SL_unit_test";

/* Vittual address of shared memory in current process */
int * vshmUnitTest =NULL;

int getFBInfor(int * pfbWidth, int * pfbHeight, int * pfbBPP )
{
	int ret, fd_fb;
	struct fb_var_screeninfo fb_var;
	struct fb_fix_screeninfo fb_fix;

	ret = 0;
	/* get fb info */
	if ((fd_fb = open(PRIMARYFBDEV, O_RDWR, 0)) < 0) {
		printf("Unable to open /dev/fb0\n");
		ret = -1;
		goto getFBInfor_err1;
	}

	if ( ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix) < 0) {
		printf("Get FB fix info failed!\n");
		ret = -1;
		goto getFBInfor_err2;
	}

	if ( ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var) < 0) {
		printf("Get FB var info failed!\n");
		ret = -1;
		goto getFBInfor_err2;
	}
	*pfbWidth	= fb_var.xres;
	*pfbHeight	= fb_var.yres;
	*pfbBPP		= fb_var.bits_per_pixel;
getFBInfor_err2:
	close(fd_fb);
getFBInfor_err1:
	return ret;
}
void * first_layer_thread_func(void *arg)
{
	int i, ret;
	ScreenLayer first_layer;
	int fb_width, fb_height, fb_bpp;
	int op_type = NO_OV;

	if(arg)
		op_type = *((int *)arg);

	ret = getFBInfor(&fb_width, &fb_height, &fb_bpp);
	if(ret == -1)
	{
		printf("Can not get fb information. \n");
		goto err;
	}

	memset(&first_layer, 0, sizeof(ScreenLayer));
	if (fb_bpp == 24)
		first_layer.fmt = v4l2_fourcc('B', 'G', 'R', '3');
	else
		first_layer.fmt = v4l2_fourcc('R', 'G', 'B', 'P');

	if (op_type & COPY_TV)
		first_layer.fmt = v4l2_fourcc('U', 'Y', 'V', 'Y');

	memcpy(first_layer.fbdev, PRIMARYFBDEV, strlen(PRIMARYFBDEV)+1);
	first_layer.pPrimary = NULL;
	if ((ret = CreateScreenLayer(&first_layer, BUFCNT_1ST))
		!= E_RET_SUCCESS) {
		printf("CreateScreenLayer first layer err %d\n", ret);
		goto err;
	}

	i = 0;
	while (i < 50 && !ctrl_c_rev) {
		if (i % 3 == 0)
			memset(first_layer.bufVaddr[0], 0x0, first_layer.bufSize);
		else if (i % 3 == 1)
			memset(first_layer.bufVaddr[0], 0x80, first_layer.bufSize);
		else if (i % 3 == 2)
			memset(first_layer.bufVaddr[0], 0xff, first_layer.bufSize);
		i++;
		if (op_type & DP_LOC_SEP_ALP_OV) {
			if ((ret = UpdateScreenLayer(&first_layer)) != E_RET_SUCCESS) {
				printf("UpdateScreenLayer err %d\n",ret);
				goto err1;
			}
		} else {
			sem_wait(semIDUnit);
			if (!vshmUnitTest[1] && !vshmUnitTest[2]) {
				if ((ret = UpdateScreenLayer(&first_layer)) != E_RET_SUCCESS) {
					printf("UpdateScreenLayer err %d\n",ret);
					goto err1;
				}
			}
			sem_post(semIDUnit);
		}
		sleep(2);
	}
err1:
	while(vshmUnitTest[1] || vshmUnitTest[2])sleep(1);
	DestoryScreenLayer(&first_layer);
err:
	printf("First layer has been destroyed! \n");
	return NULL;
}

void * second_layer_thread_func(void *arg)
{
	ScreenLayer second_layer;
	LoadParam param;
	LoadParam sec_param;
	dma_addr_t paddr_2nd[BUFCNT_2ND];
	void * buf_2nd[BUFCNT_2ND];
	MethodAlphaData alpha_data;
	MethodColorKeyData colorkey_data;
	int buf_size, alpha_buf_size, x, y, i, ret, op_type = *((int *)arg);
	int SL_width, SL_height;
	char alpha_val;
	int fb_width, fb_height, fb_bpp;
	int show_time = 0;
	int load_time = 0;
	int update_time = 0;
	int bufcnt = BUFCNT_2ND;

	ret = getFBInfor(&fb_width, &fb_height, &fb_bpp);
	if(ret == -1)
	{
		printf("Can not get fb information. \n");
		goto err;
	}

	memset(&second_layer, 0, sizeof(ScreenLayer));

	i=100;
	while(i)
	{
		i--;
		second_layer.pPrimary = GetPrimarySLHandle(PRIMARYFBDEV);
		if(second_layer.pPrimary == NULL)
			usleep(200*1000);
		else
			break;
	}
	if(i==0)
	{
		printf("Should create primary layer firstly. \n");
		ret = -1;
		goto err;
	}

	alpha_buf_size = 0;
	memset(&alpha_data, 0, sizeof(MethodAlphaData));
	memset(&colorkey_data, 0, sizeof(MethodColorKeyData));
	second_layer.screenRect.left = 0;
	second_layer.screenRect.top = 0;
	second_layer.screenRect.right = fb_width;
	second_layer.screenRect.bottom = fb_height;
	SL_width = second_layer.screenRect.right - second_layer.screenRect.left;
	SL_height = second_layer.screenRect.bottom - second_layer.screenRect.top;
	if (op_type & IC_LOC_PIX_ALP_OV)
		second_layer.fmt = v4l2_fourcc('R', 'G', 'B', 'A');
	else if (op_type & COPY_TV) /* for better performance */
		second_layer.fmt = v4l2_fourcc('U', 'Y', 'V', 'Y');
	else
		second_layer.fmt = v4l2_fourcc('B', 'G', 'R', '3');
	if (op_type & IC_LOC_SEP_ALP_OV)
		second_layer.supportSepLocalAlpha = 1;
	if ((ret = CreateScreenLayer(&second_layer, bufcnt))
		!= E_RET_SUCCESS) {
		printf("CreateScreenLayer second layer err %d\n", ret);
		goto err;
	}
	/* set alpha and key color */
	if (op_type & IC_GLB_ALP_OV) {
		alpha_data.globalAlphaEnable = 1;
		alpha_data.alpha = 255;
	} else if (op_type & IC_LOC_SEP_ALP_OV)
		alpha_data.sepLocalAlphaEnable = 1;
	colorkey_data.enable = 0;
	colorkey_data.keyColor = 0;
	if ((ret = SetScreenLayer(&second_layer, E_SET_ALPHA, &alpha_data))
		!= E_RET_SUCCESS) {
		printf("SetScreenLayer E_SET_ALPHA for second layer err %d\n", ret);
		goto err1;
	}
	if ((ret = SetScreenLayer(&second_layer, E_SET_COLORKEY, &colorkey_data))
		!= E_RET_SUCCESS) {
		printf("SetScreenLayer E_SET_COLORKEY for second layer err %d\n", ret);
		goto err1;
	}

	if (op_type & COPY_TV) {
		MethodTvoutData tvout;
		tvout.tvMode = TVOUT_NTSC;
		tvout.lcd2tvRotation = 0;
		if ((ret = SetScreenLayer(&second_layer, E_COPY_TVOUT, &tvout))
				!= E_RET_SUCCESS) {
			printf("SetScreenLayer E_ENABLE_TVTOU for second layer err %d\n", ret);
			goto err1;
		}
	}

	param.srcWidth = 320;
	param.srcHeight = 240;
	param.srcFmt = v4l2_fourcc('I', '4', '2', '0');
	param.srcRect.left = 0;
	param.srcRect.top = 0;
	param.srcRect.right = 320;
	param.srcRect.bottom = 240;
	param.destRect.left = 0;
	param.destRect.top = 0;
	param.destRect.right = second_layer.screenRect.right - second_layer.screenRect.left;
	param.destRect.bottom = second_layer.screenRect.bottom - second_layer.screenRect.top;
	param.destRot = 0;

	/* just add third layer to this screenlayer to show the performance*/
	if (op_type & COPY_TV){
		sec_param.srcWidth = 320;
		sec_param.srcHeight = 240;
		sec_param.srcFmt = v4l2_fourcc('I', '4', '2', '0');
		sec_param.srcRect.left = 0;
		sec_param.srcRect.top = 0;
		sec_param.srcRect.right = 320;
		sec_param.srcRect.bottom = 240;
		sec_param.destRect.left = (second_layer.screenRect.right - second_layer.screenRect.left)/2;
		sec_param.destRect.top = 0;
		sec_param.destRect.right = second_layer.screenRect.right - second_layer.screenRect.left;
		sec_param.destRect.bottom = (second_layer.screenRect.bottom - second_layer.screenRect.top)/2;
		sec_param.destRot = 0;
	}

	buf_size = param.srcWidth * param.srcHeight * 3/2;
	if (op_type & IC_LOC_SEP_ALP_OV)
		alpha_buf_size = SL_width * SL_height;
	ret = dma_memory_alloc(buf_size, bufcnt, paddr_2nd, buf_2nd);
	if ( ret < 0) {
		printf("dma_memory_alloc failed\n");
		goto err1;
	}

	for (i=0;i<FRM_CNT;i++) {
		struct timeval frame_begin,frame_end;
		struct timeval load_begin,load_end;
		struct timeval update_begin,update_end;
                int sec, usec;

		if (ctrl_c_rev)
			break;

		param.srcPaddr = paddr_2nd[i%bufcnt];
		gen_fill_pattern(buf_2nd[i%bufcnt], param.srcWidth, param.srcHeight);

		gettimeofday(&frame_begin, NULL);
		gettimeofday(&load_begin, NULL);

		if ((ret = LoadScreenLayer(&second_layer, &param, i%bufcnt)) != E_RET_SUCCESS) {
			printf("LoadScreenLayer err %d\n", ret);
			goto err2;
		}

		if (op_type & COPY_TV){
			sec_param.srcPaddr = paddr_2nd[i%bufcnt];
			if ((ret = LoadScreenLayer(&second_layer, &sec_param, i%bufcnt)) != E_RET_SUCCESS) {
				printf("LoadScreenLayer sec err %d\n", ret);
				goto err2;
			}
		}
		gettimeofday(&load_end, NULL);

		sec = load_end.tv_sec - load_begin.tv_sec;
		usec = load_end.tv_usec - load_begin.tv_usec;

		if (usec < 0) {
			sec--;
			usec = usec + 1000000;
		}
		load_time += (sec * 1000000) + usec;

		gettimeofday(&frame_end, NULL);

                sec = frame_end.tv_sec - frame_begin.tv_sec;
                usec = frame_end.tv_usec - frame_begin.tv_usec;

                if (usec < 0) {
                        sec--;
                        usec = usec + 1000000;
                }
                show_time += (sec * 1000000) + usec;

		/* Fill local alpha buffer */
		if (op_type & IC_LOC_SEP_ALP_OV) {
			gen_fill_alpha_in_separate_buffer(second_layer.bufAlphaVaddr[i%bufcnt], alpha_buf_size, 0x80);
			if (i < FRM_CNT/3) {
				alpha_val = 0x80;
				for (x=SL_width/4; x<3*SL_width/4; x++)
					for (y=SL_height/4; y<3*SL_height/4; y++)
						LoadAlphaPoint(&second_layer, x, y, alpha_val, i%bufcnt);
			} else if (i < 2*FRM_CNT/3) {
				alpha_val = 0x00;
				for (x=SL_width/4; x<3*SL_width/4; x++)
					for (y=SL_height/4; y<3*SL_height/4; y++)
						LoadAlphaPoint(&second_layer, x, y, alpha_val, i%bufcnt);
			} else if (i < FRM_CNT) {
				alpha_val = 0xFF;
				for (x=SL_width/4; x<3*SL_width/4; x++)
					for (y=SL_height/4; y<3*SL_height/4; y++)
						LoadAlphaPoint(&second_layer, x, y, alpha_val, i%bufcnt);
			}
		}
		/* Change local alpha value in pixel */
		else if (op_type & IC_LOC_PIX_ALP_OV) {
			gen_fill_alpha_in_pixel(second_layer.bufVaddr[i%bufcnt], second_layer.fmt, second_layer.bufSize, 0x80);
			if (i < FRM_CNT/3) {
				alpha_val = 0x80;
				for (x=SL_width/4; x<3*SL_width/4; x++)
					for (y=SL_height/4; y<3*SL_height/4; y++)
						gen_fill_alpha_in_pixel_for_point(second_layer.bufVaddr[i%bufcnt], second_layer.fmt, SL_width, x, y, alpha_val);
			} else if (i < 2*FRM_CNT/3) {
				alpha_val = 0x00;
				for (x=SL_width/4; x<3*SL_width/4; x++)
					for (y=SL_height/4; y<3*SL_height/4; y++)
						gen_fill_alpha_in_pixel_for_point(second_layer.bufVaddr[i%bufcnt], second_layer.fmt, SL_width, x, y, alpha_val);
			} else if (i < FRM_CNT) {
				alpha_val = 0xFF;
				for (x=SL_width/4; x<3*SL_width/4; x++)
					for (y=SL_height/4; y<3*SL_height/4; y++)
						gen_fill_alpha_in_pixel_for_point(second_layer.bufVaddr[i%bufcnt], second_layer.fmt, SL_width, x, y, alpha_val);
			}
		}

		gettimeofday(&frame_begin, NULL);
		gettimeofday(&update_begin, NULL);

		if ((ret = FlipScreenLayerBuf(&second_layer, i%bufcnt)) != E_RET_SUCCESS) {
			printf("FlipScreenLayerBuf err %d\n", ret);
			goto err2;
		}

		sem_wait(semIDUnit);
		if (!vshmUnitTest[2]) {
			if ((ret = UpdateScreenLayer(&second_layer)) != E_RET_SUCCESS) {
				printf("UpdateScreenLayer err %d\n",ret);
				goto err2;
			}
		}
		sem_post(semIDUnit);

		gettimeofday(&update_end, NULL);

		sec = update_end.tv_sec - update_begin.tv_sec;
		usec = update_end.tv_usec - update_begin.tv_usec;

		if (usec < 0) {
			sec--;
			usec = usec + 1000000;
		}
		update_time += (sec * 1000000) + usec;

		gettimeofday(&frame_end, NULL);

                sec = frame_end.tv_sec - frame_begin.tv_sec;
                usec = frame_end.tv_usec - frame_begin.tv_usec;

                if (usec < 0) {
                        sec--;
                        usec = usec + 1000000;
                }
                show_time += (sec * 1000000) + usec;
	}

	printf("2nd layer load avg frame time %d us\n", load_time/i);
	printf("2nd layer update avg frame time %d us\n", update_time/i);
	printf("2nd layer avg frame time %d us\n", show_time/i);

err2:
	dma_memory_free(buf_size, bufcnt, paddr_2nd, buf_2nd);
err1:
	DestoryScreenLayer(&second_layer);
err:
	sem_wait(semIDUnit);
	vshmUnitTest[1] = 0;
	sem_post(semIDUnit);
	printf("Second layer has been destroyed! \n");
	return NULL;
}

void * third_layer_thread_func(void *arg)
{
	ScreenLayer third_layer;
	LoadParam param;
	dma_addr_t paddr_3th[BUFCNT_3TH];
	void * buf_3th[BUFCNT_3TH];
	MethodAlphaData alpha_data;
	MethodColorKeyData colorkey_data;
	int buf_size, alpha_buf_size, i, ret, x, y, op_type = *((int *)arg);
	int SL_width, SL_height;
	char alpha_val;
	int fb_width, fb_height, fb_bpp;
	int show_time = 0;

	ret = getFBInfor(&fb_width, &fb_height, &fb_bpp);
	if(ret == -1)
	{
		printf("Can not get fb information. \n");
		goto err;
	}

	alpha_buf_size = 0;
	memset(&third_layer, 0, sizeof(ScreenLayer));
	memset(&alpha_data, 0, sizeof(MethodAlphaData));
	memset(&colorkey_data, 0, sizeof(MethodColorKeyData));
	third_layer.screenRect.left = fb_width*1/4;
	third_layer.screenRect.top = fb_height*1/4;
	third_layer.screenRect.right = fb_width*3/4;
	third_layer.screenRect.bottom = fb_height*3/4;
	SL_width = third_layer.screenRect.right - third_layer.screenRect.left;
	SL_height = third_layer.screenRect.bottom - third_layer.screenRect.top;
	if (op_type & IC_LOC_PIX_ALP_OV)
		third_layer.fmt = v4l2_fourcc('R', 'G', 'B', 'A');
	else
		third_layer.fmt = v4l2_fourcc('R', 'G', 'B', 'P');
	i=100;
	while(i)
	{
		i--;
		third_layer.pPrimary = GetPrimarySLHandle(PRIMARYFBDEV);
		if(third_layer.pPrimary == NULL)
			usleep(20*1000);
		else
			break;
	}
	if(i==0)
		printf("Should create primary layer firstly. \n");
	if (op_type & IC_LOC_SEP_ALP_OV)
		third_layer.supportSepLocalAlpha = 1;
	if ((ret = CreateScreenLayer(&third_layer, BUFCNT_3TH))
		!= E_RET_SUCCESS) {
		printf("CreateScreenLayer third layer err %d\n", ret);
		goto err;
	}
	/* set alpha and key color */
	if (op_type & IC_GLB_ALP_OV) {
		alpha_data.globalAlphaEnable = 1;
		alpha_data.alpha = 255;
	} else if (op_type & IC_LOC_SEP_ALP_OV)
		alpha_data.sepLocalAlphaEnable = 1;
	colorkey_data.enable = 0;
	colorkey_data.keyColor = 0;
	if ((ret = SetScreenLayer(&third_layer, E_SET_ALPHA, &alpha_data))
		!= E_RET_SUCCESS) {
		printf("SetScreenLayer E_SET_ALPHA for third layer err %d\n", ret);
		goto err1;
	}
	if ((ret = SetScreenLayer(&third_layer, E_SET_COLORKEY, &colorkey_data))
		!= E_RET_SUCCESS) {
		printf("SetScreenLayer E_SET_COLORKEY for third layer err %d\n", ret);
		goto err1;
	}

	param.srcWidth = 320;
	param.srcHeight = 240;
	param.srcFmt = v4l2_fourcc('I', '4', '2', '0');
	param.srcRect.left = 0;
	param.srcRect.top = 0;
	param.srcRect.right = 320;
	param.srcRect.bottom = 240;
	param.destRect.left = 0;
	param.destRect.top = 0;
	param.destRect.right = third_layer.screenRect.right - third_layer.screenRect.left;
	param.destRect.bottom = third_layer.screenRect.bottom - third_layer.screenRect.top;
	param.destRot = 0;
	buf_size = param.srcWidth * param.srcHeight * 3/2;
	if (op_type & IC_LOC_SEP_ALP_OV)
		alpha_buf_size = SL_width * SL_height;
	ret = dma_memory_alloc(buf_size, BUFCNT_3TH, paddr_3th, buf_3th);
	if ( ret < 0) {
		printf("dma_memory_alloc failed\n");
		goto err1;
	}

	for (i=0;i<FRM_CNT;i++) {
		struct timeval frame_begin,frame_end;
                int sec, usec;

		if (ctrl_c_rev)
			break;

		param.srcPaddr = paddr_3th[i%BUFCNT_3TH];
		gen_fill_pattern(buf_3th[i%BUFCNT_3TH], param.srcWidth, param.srcHeight);

		gettimeofday(&frame_begin, NULL);

		if ((ret = LoadScreenLayer(&third_layer, &param, i%BUFCNT_3TH)) != E_RET_SUCCESS) {
			printf("LoadScreenLayer err %d\n", ret);
			goto err2;
		}

		gettimeofday(&frame_end, NULL);

                sec = frame_end.tv_sec - frame_begin.tv_sec;
                usec = frame_end.tv_usec - frame_begin.tv_usec;

                if (usec < 0) {
                        sec--;
                        usec = usec + 1000000;
                }
                show_time += (sec * 1000000) + usec;

		/* Fill local alpha buffer */
		if (op_type & IC_LOC_SEP_ALP_OV) {
			if(i < FRM_CNT/3)
				gen_fill_alpha_in_separate_buffer(third_layer.bufAlphaVaddr[i%BUFCNT_3TH], alpha_buf_size, 0x80);
			else if(i < 2*FRM_CNT/3)
				gen_fill_alpha_in_separate_buffer(third_layer.bufAlphaVaddr[i%BUFCNT_3TH], alpha_buf_size, 0x00);
			else if(i < FRM_CNT)
				gen_fill_alpha_in_separate_buffer(third_layer.bufAlphaVaddr[i%BUFCNT_3TH], alpha_buf_size, 0xFF);
		}
		/* Change local alpha value in pixel */
		else if (op_type & IC_LOC_PIX_ALP_OV) {
			gen_fill_alpha_in_pixel(third_layer.bufVaddr[i%BUFCNT_3TH], third_layer.fmt, third_layer.bufSize, 0x80);
			if (i < FRM_CNT/3) {
				alpha_val = 0x80;
				for (x=SL_width/4; x<3*SL_width/4; x++)
					for (y=SL_height/4; y<3*SL_height/4; y++)
						gen_fill_alpha_in_pixel_for_point(third_layer.bufVaddr[i%BUFCNT_3TH], third_layer.fmt, SL_width, x, y, alpha_val);
			} else if (i < 2*FRM_CNT/3) {
				alpha_val = 0x00;
				for (x=SL_width/4; x<3*SL_width/4; x++)
					for (y=SL_height/4; y<3*SL_height/4; y++)
						gen_fill_alpha_in_pixel_for_point(third_layer.bufVaddr[i%BUFCNT_3TH], third_layer.fmt, SL_width, x, y, alpha_val);
			} else if (i < FRM_CNT) {
				alpha_val = 0xFF;
				for (x=SL_width/4; x<3*SL_width/4; x++)
					for (y=SL_height/4; y<3*SL_height/4; y++)
						gen_fill_alpha_in_pixel_for_point(third_layer.bufVaddr[i%BUFCNT_3TH], third_layer.fmt, SL_width, x, y, alpha_val);
			}
		}

		gettimeofday(&frame_begin, NULL);

		if ((ret = FlipScreenLayerBuf(&third_layer, i%BUFCNT_3TH)) != E_RET_SUCCESS) {
			printf("FlipScreenLayerBuf err %d\n", ret);
			goto err2;
		}

		if ((ret = UpdateScreenLayer(&third_layer)) != E_RET_SUCCESS) {
			printf("UpdateScreenLayer err %d\n",ret);
			goto err2;
		}

		gettimeofday(&frame_end, NULL);

                sec = frame_end.tv_sec - frame_begin.tv_sec;
                usec = frame_end.tv_usec - frame_begin.tv_usec;

                if (usec < 0) {
                        sec--;
                        usec = usec + 1000000;
                }
                show_time += (sec * 1000000) + usec;

                /* screenlayer should not update fast than lcd refresh rate*/
                usleep(10000);
	}

	printf("3rd layer avg frame time %d us\n", show_time/i);

err2:
	dma_memory_free(buf_size, BUFCNT_3TH, paddr_3th, buf_3th);
err1:
	DestoryScreenLayer(&third_layer);
err:
	sem_wait(semIDUnit);
	vshmUnitTest[2] = 0;
	sem_post(semIDUnit);
	printf("Third layer has been destroyed! \n");
	return NULL;
}

int screenlayer_test(int three_layer, int op_type)
{
	pthread_t first_layer_thread;
	pthread_t second_layer_thread;
	pthread_t third_layer_thread;
	int ret;
        int shmIDUnit;
        struct stat shmStat;
        char shmNameUnit[32]="shm_name_unit_test";

        ret = 0;
        semIDUnit = sem_open(semNameUnit, O_CREAT, 0666, 1);
        if(SEM_FAILED == semIDUnit){
                printf("can not open the semaphore for SL IPC Unit test!\n");
                ret = -1;
                goto done;
        }

        shmIDUnit = shm_open(shmNameUnit, O_RDWR|O_CREAT, 0666);
        if(shmIDUnit == -1){
                printf("can not open the shared memory for SL IPC Unit test!\n");
                ret = -1;
                sem_close(semIDUnit);
                goto done;
        }
        /* Get special size shm */
        ftruncate(shmIDUnit,3 * sizeof(int));
        /* Connect to the shm */
        fstat(shmIDUnit, &shmStat);

        if(vshmUnitTest == NULL)
                vshmUnitTest = (int *)mmap(NULL,shmStat.st_size,PROT_READ|PROT_WRITE,MAP_SHARED,shmIDUnit,0);

        if(vshmUnitTest == MAP_FAILED || vshmUnitTest ==NULL)
        {
                ret = -1;
                sem_close(semIDUnit);
                shm_unlink(shmNameUnit);
                goto done;
        }
	memset(vshmUnitTest, 0, 3 *sizeof(int));

	/* create first layer */
	printf("Display to primary layer!\n");
	pthread_create(&first_layer_thread, NULL, first_layer_thread_func, NULL);

	/* create second layer */
	printf("Add second layer!\n");
	vshmUnitTest[1] = 1;
	pthread_create(&second_layer_thread, NULL, second_layer_thread_func, &op_type);

	if (three_layer) {
		/* create third layer */
		printf("Add third layer!\n");
		vshmUnitTest[2] = 1;
		pthread_create(&third_layer_thread, NULL, third_layer_thread_func, &op_type);
	}

	while(!ctrl_c_rev)usleep(100000);

	if (three_layer)
		pthread_join(third_layer_thread, NULL);
	pthread_join(second_layer_thread, NULL);
	pthread_join(first_layer_thread, NULL);

	sem_unlink(semNameUnit);
	shm_unlink(shmNameUnit);
done:
	return ret;
}
/*
**	ProcessA: First_layer & second_layer
**	ProcessB: Third_layer
*/
int screenlayer_test_ipc(int process_num, int three_layer, int op_type)
{
	pthread_t first_layer_thread;
	pthread_t second_layer_thread;
	pthread_t third_layer_thread;
	int ret;
	int shmIDUnit;
	struct stat shmStat;
	char shmNameUnit[32]="shm_name_unit_test";

	ret = 0;
	semIDUnit = sem_open(semNameUnit, O_CREAT, 0666, 1);
	if(SEM_FAILED == semIDUnit){
                printf("can not open the semaphore for SL IPC Unit test!\n");
                ret = -1;
                goto done;
        }

	shmIDUnit = shm_open(shmNameUnit, O_RDWR|O_CREAT, 0666);
        if(shmIDUnit == -1){
                printf("can not open the shared memory for SL IPC Unit test!\n");
                ret = -1;
                sem_close(semIDUnit);
                goto done;
        }
        /* Get special size shm */
        ftruncate(shmIDUnit,3 * sizeof(int));
        /* Connect to the shm */
        fstat(shmIDUnit, &shmStat);

        if(vshmUnitTest == NULL)
		vshmUnitTest = (int *)mmap(NULL,shmStat.st_size,PROT_READ|PROT_WRITE,MAP_SHARED,shmIDUnit,0);

	if(vshmUnitTest == MAP_FAILED || vshmUnitTest ==NULL)
	{
		ret = -1;
		sem_close(semIDUnit);
		shm_unlink(shmNameUnit);
		goto done;
	}
	memset(vshmUnitTest, 0, 3 *sizeof(int));
	vshmUnitTest[1] = 1;

	/* Two process cases*/
	if(process_num == 2)
	{
		if(three_layer) vshmUnitTest[2]  = 1;
		/* Process A */
		if(fork() != 0)
		{
			/* create first layer */
			printf("Display to primary layer!\n");
			pthread_create(&first_layer_thread, NULL, first_layer_thread_func, &op_type);

			if(three_layer)
			{
				/* create second layer */
				sleep(2);
				printf("Add second layer!\n");
				pthread_create(&second_layer_thread, NULL, second_layer_thread_func, &op_type);
				pthread_join(second_layer_thread, NULL);
			}

			pthread_join(first_layer_thread, NULL);
			sem_unlink(semNameUnit);
			shm_unlink(shmNameUnit);
			printf("SL IPC: Process A has been exited. \n");
			exit(0);
		}else{
			/* Process B */
			sleep(2);
			if(three_layer)
			{
				/* create third layer */
				printf("Add third layer!\n");
				pthread_create(&third_layer_thread, NULL, third_layer_thread_func, &op_type);
				pthread_join(third_layer_thread, NULL);
			} else
			{
				/* create second layer */
		                printf("Add second layer(two layers case)!\n");
                        	pthread_create(&second_layer_thread, NULL, second_layer_thread_func, &op_type);
		                pthread_join(second_layer_thread, NULL);
			}
			printf("SL IPC: Process B has been exited. \n");
		}
		sem_unlink(semNameUnit);
		shm_unlink(shmNameUnit);
		exit(0);
	}else if(process_num ==3)
	{
		vshmUnitTest[2]  = 1;
		/* Process A */
                if(fork()!=0)
                {
                        /* create first layer */
                        printf("Display to primary layer!\n");
                        pthread_create(&first_layer_thread, NULL, first_layer_thread_func, NULL);
                        pthread_join(first_layer_thread, NULL);
                        sem_unlink(semNameUnit);
                        shm_unlink(shmNameUnit);
                        printf("SL IPC: Process A has been exited. \n");
                        exit(0);
                }else{
			/* Process B */
			if(fork()==0)
			{
                                /* create second layer */
				sleep(2);
                                printf("Add second layer!\n");
                                pthread_create(&second_layer_thread, NULL, second_layer_thread_func, &op_type);
				pthread_join(second_layer_thread, NULL);
				sem_unlink(semNameUnit);
	                        shm_unlink(shmNameUnit);
        	                printf("SL IPC: Process B has been exited. \n");
                	        exit(0);
			}
			else
			{
				/* Process C */
				/* create third layer */
				sleep(2);
				printf("Add third layer!\n");
				pthread_create(&third_layer_thread, NULL, third_layer_thread_func, &op_type);
				pthread_join(third_layer_thread, NULL);
				sem_unlink(semNameUnit);
	                        shm_unlink(shmNameUnit);
        	                printf("SL IPC: Process C has been exited. \n");
                	        exit(0);
			}

		}
	}

done:
	return ret;
}

int run_test_pattern(int pattern, ipu_test_handle_t * test_handle)
{
	if (pattern == 1) {
		printf("Color bar test with full-screen:\n");
		return color_bar(0, NO_OV, test_handle);
	}
	if (pattern == 2) {
		printf("Color bar test 1 input with 2 output:\n");
		return color_bar(1, NO_OV, test_handle);
	}
	if (pattern == 3) {
		printf("Hopping block test:\n");
		return hop_block(test_handle);
	}
	if (pattern == 4) {
		int ret=0;
		int ret1=0;
		int ret2=0;
		pthread_t thread1;
		pthread_t thread2;

		printf("Hopping block + color bar thread test:\n");

		pthread_create(&thread2, NULL, thread_func_hop_block, test_handle);
		pthread_create(&thread1, NULL, thread_func_color_bar, NULL);

		sleep(1);
		pthread_join(thread1, (void*)(&ret1));
		pthread_join(thread2, (void*)(&ret2));
		printf("Hopping block + color bar threads has joied:\n");
		if(ret1==0 && ret2==0)
			ret = 0;
		else
			ret = -1;
		return ret;
	}
	if (pattern == 5) {
		printf("Color bar IC global alpha overlay test:\n");
		return color_bar(0, IC_GLB_ALP_OV, test_handle);
	}
	if (pattern == 6) {
		printf("Color bar IC separate local alpha overlay test:\n");
		return color_bar(0, IC_LOC_SEP_ALP_OV, test_handle);
	}
	if (pattern == 7) {
		printf("Color bar IC local alpha within pixel overlay test:\n");
		return color_bar(0, IC_LOC_PIX_ALP_OV, test_handle);
	}
	if (pattern == 8) {
		printf("Copy test:\n");
		return copy_test(test_handle);
	}
	if (pattern == 9) {
		printf("Screen layer with 2 layers test using IC global alpha blending:\n");
		return screenlayer_test(0, IC_GLB_ALP_OV);
	}
	if (pattern == 10) {
		printf("Screen layer with 3 layers test using IC global alpha blending:\n");
		return screenlayer_test(1, IC_GLB_ALP_OV);
	}
	if (pattern == 11) {
		printf("Screen layer with 2 layers test using IC local alpha blending with alpha value in separate buffer:\n");
		return screenlayer_test(0, IC_LOC_SEP_ALP_OV);
	}
	if (pattern == 12) {
		printf("Screen layer with 3 layers test using IC local alpha blending with alpha value in separate buffer:\n");
		return screenlayer_test(1, IC_LOC_SEP_ALP_OV);
 	}
	if (pattern == 13) {
		printf("Screen layer with 2 layers test using IC local alpha blending with alpha value in pixel:\n");
		return screenlayer_test(0, IC_LOC_PIX_ALP_OV);
	}
	if (pattern == 14) {
		printf("Screen layer with 3 layers test using IC local alpha blending with alpha value in pixel:\n");
		return screenlayer_test(1, IC_LOC_PIX_ALP_OV);
 	}
	if (pattern == 15){
		printf("Screen layer IPC(global alpha): ProcessA(first_layer ) ProcessB(second_layer):\n");
		return screenlayer_test_ipc(2, 0, IC_GLB_ALP_OV);
	}
	if (pattern == 16){
		printf("Screen layer IPC(local alpha): ProcessA(first_layer ) ProcessB(second_layer):\n");
		return screenlayer_test_ipc(2, 0, IC_LOC_SEP_ALP_OV);
	}
	if (pattern == 17){
		printf("Screen layer IPC(global alpha): ProcessA(first_layer + second_layer) ProcessB(third_layer):\n");
		return screenlayer_test_ipc(2, 1, IC_GLB_ALP_OV);
	}
	if (pattern == 18){
		printf("Screen layer IPC(local alpha): ProcessA(first_layer + second_layer) ProcessB(third_layer):\n");
		return screenlayer_test_ipc(2, 1, IC_LOC_SEP_ALP_OV);
	}
	if (pattern == 19){
		printf("Screen layer IPC(local alpha): ProcessA(first_layer) ProcessB(second_layer) ProcessC(third_layer):\n");
		return screenlayer_test_ipc(3, 1, IC_LOC_SEP_ALP_OV);
	}
	if (pattern == 20){
		printf("[No mandatory test]Screen layer IPC(local alpha + tvout): ProcessA(first_layer) ProcessB(second_layer):\n");
		return screenlayer_test_ipc(2, 0, IC_LOC_SEP_ALP_OV | COPY_TV);
	}
	if (pattern == 21){
		printf("Horizontally slipped video test on TV(support upsizing), assuming the TV uses MEM_DC_SYNC channel:\n");
		return h_splitted_tv_video_playback(test_handle);
	}

	printf("No such test pattern %d\n", pattern);
	return -1;
}
