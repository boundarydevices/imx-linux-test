/*
 * Copyright 2009-2012 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file mxc_ipudev_test.c
 *
 * @brief IPU device lib test implementation
 *
 * @ingroup IPU
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/mxcfb.h>
#include "mxc_ipudev_test.h"

#define PAGE_ALIGN(x) (((x) + 4095) & ~4095)
int ctrl_c_rev = 0;
void ctrl_c_handler(int signum, siginfo_t *info, void *myact)
{
	ctrl_c_rev = 1;
}

int process_cmdline(int argc, char **argv, ipu_test_handle_t *test_handle)
{
	int i;
	int pre_set = 0;
	struct ipu_task *t = &test_handle->task;

	if (argc == 1)
		return -1;

	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], "-C") == 0)
		{
			pre_set = 1;
			parse_config_file(argv[++i], test_handle);
    }

	 if(pre_set == 0)
		 parse_cmd_input(argc,argv,test_handle);

	if ((t->input.width == 0) || (t->input.height == 0) ||
			(t->output.width == 0) ||
			(t->output.height == 0)
			|| (test_handle->fcount < 1))
		return -1;

	return 0;
}

static unsigned int fmt_to_bpp(unsigned int pixelformat)
{
        unsigned int bpp;

        switch (pixelformat)
        {
                case IPU_PIX_FMT_RGB565:
                /*interleaved 422*/
                case IPU_PIX_FMT_YUYV:
                case IPU_PIX_FMT_UYVY:
                /*non-interleaved 422*/
                case IPU_PIX_FMT_YUV422P:
                case IPU_PIX_FMT_YVU422P:
                        bpp = 16;
                        break;
                case IPU_PIX_FMT_BGR24:
                case IPU_PIX_FMT_RGB24:
                case IPU_PIX_FMT_YUV444:
                        bpp = 24;
                        break;
                case IPU_PIX_FMT_BGR32:
                case IPU_PIX_FMT_BGRA32:
                case IPU_PIX_FMT_RGB32:
                case IPU_PIX_FMT_RGBA32:
                case IPU_PIX_FMT_ABGR32:
                        bpp = 32;
                        break;
                /*non-interleaved 420*/
                case IPU_PIX_FMT_YUV420P:
                case IPU_PIX_FMT_YVU420P:
                case IPU_PIX_FMT_YUV420P2:
                case IPU_PIX_FMT_NV12:
		case IPU_PIX_FMT_TILED_NV12:
                        bpp = 12;
                        break;
                default:
                        bpp = 8;
                        break;
        }
        return bpp;
}

static void dump_ipu_task(struct ipu_task *t)
{
	printf("====== ipu task ======\n");
	printf("input:\n");
	printf("\tforamt: 0x%x\n", t->input.format);
	printf("\twidth: %d\n", t->input.width);
	printf("\theight: %d\n", t->input.height);
	printf("\tcrop.w = %d\n", t->input.crop.w);
	printf("\tcrop.h = %d\n", t->input.crop.h);
	printf("\tcrop.pos.x = %d\n", t->input.crop.pos.x);
	printf("\tcrop.pos.y = %d\n", t->input.crop.pos.y);
	if (t->input.deinterlace.enable) {
		printf("deinterlace enabled with:\n");
		if (t->input.deinterlace.motion != HIGH_MOTION)
			printf("\tlow/medium motion\n");
		else
			printf("\thigh motion\n");
	}
	printf("output:\n");
	printf("\tforamt: 0x%x\n", t->output.format);
	printf("\twidth: %d\n", t->output.width);
	printf("\theight: %d\n", t->output.height);
	printf("\troate: %d\n", t->output.rotate);
	printf("\tcrop.w = %d\n", t->output.crop.w);
	printf("\tcrop.h = %d\n", t->output.crop.h);
	printf("\tcrop.pos.x = %d\n", t->output.crop.pos.x);
	printf("\tcrop.pos.y = %d\n", t->output.crop.pos.y);
	if (t->overlay_en) {
		printf("overlay:\n");
		printf("\tforamt: 0x%x\n", t->overlay.format);
		printf("\twidth: %d\n", t->overlay.width);
		printf("\theight: %d\n", t->overlay.height);
		printf("\tcrop.w = %d\n", t->overlay.crop.w);
		printf("\tcrop.h = %d\n", t->overlay.crop.h);
		printf("\tcrop.pos.x = %d\n", t->overlay.crop.pos.x);
		printf("\tcrop.pos.y = %d\n", t->overlay.crop.pos.y);
		if (t->overlay.alpha.mode == IPU_ALPHA_MODE_LOCAL)
			printf("combine with local alpha\n");
		else
			printf("combine with global alpha %d\n", t->overlay.alpha.gvalue);
		if (t->overlay.colorkey.enable)
			printf("colorkey enabled with 0x%x\n", t->overlay.colorkey.value);
	}
}

int main(int argc, char *argv[])
{
	ipu_test_handle_t test_handle;
	struct ipu_task *t = &test_handle.task;
	int ret = 0, done_cnt = 0;
	int done_loop = 0, total_cnt = 0;
	struct sigaction act;
	FILE * file_in = NULL;
	FILE * file_out = NULL;
	struct timeval begin, end;
	int sec, usec, run_time = 0;
	int fd_ipu = 0, fd_fb = 0;
	int isize = 0, ovsize = 0;
	int alpsize = 0, osize = 0;
	void *inbuf = NULL, *vdibuf = NULL;
	void *ovbuf = NULL, *alpbuf = NULL;
	void *outbuf = NULL;
	dma_addr_t outpaddr;
	struct fb_var_screeninfo fb_var;
	struct fb_fix_screeninfo fb_fix;
	int blank;

	/*for ctrl-c*/
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = ctrl_c_handler;

	if((ret = sigaction(SIGINT, &act, NULL)) < 0) {
		printf("install sigal error\n");
		return ret;
	}

	memset(&test_handle, 0, sizeof(ipu_test_handle_t));

	if (process_cmdline(argc, argv, &test_handle) < 0) {
		printf("\nMXC IPU device Test\n\n" \
			"Usage: %s -C <config file> <input raw file>\n",
			argv[0]);
		return -1;
	}

	file_in = fopen(argv[argc-1], "rb");
	if (file_in == NULL){
		printf("there is no such file for reading %s\n", argv[argc-1]);
		ret = -1;
		goto err0;
	}

	fd_ipu = open("/dev/mxc_ipu", O_RDWR, 0);
	if (fd_ipu < 0) {
		printf("open ipu dev fail\n");
		ret = -1;
		goto err1;
	}

	if (IPU_PIX_FMT_TILED_NV12F == t->input.format) {
		isize = PAGE_ALIGN(t->input.width * t->input.height/2) +
			PAGE_ALIGN(t->input.width * t->input.height/4);
		isize = t->input.paddr = isize * 2;
	} else
		isize = t->input.paddr =
			t->input.width * t->input.height
			* fmt_to_bpp(t->input.format)/8;
	ret = ioctl(fd_ipu, IPU_ALLOC, &t->input.paddr);
	if (ret < 0) {
		printf("ioctl IPU_ALLOC fail\n");
		goto err2;
	}
	inbuf = mmap(0, isize, PROT_READ | PROT_WRITE,
		MAP_SHARED, fd_ipu, t->input.paddr);
	if (!inbuf) {
		printf("mmap fail\n");
		ret = -1;
		goto err3;
	}

	if (t->input.deinterlace.enable &&
		(t->input.deinterlace.motion != HIGH_MOTION)) {
		t->input.paddr_n = isize;
		ret = ioctl(fd_ipu, IPU_ALLOC, &t->input.paddr_n);
		if (ret < 0) {
			printf("ioctl IPU_ALLOC fail\n");
			goto err4;
		}
		vdibuf = mmap(0, isize, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd_ipu, t->input.paddr_n);
		if (!vdibuf) {
			printf("mmap fail\n");
			ret = -1;
			goto err5;
		}
	}

	if (t->overlay_en) {
		ovsize = t->overlay.paddr =
			t->overlay.width * t->overlay.height
			* fmt_to_bpp(t->overlay.format)/8;
		ret = ioctl(fd_ipu, IPU_ALLOC, &t->overlay.paddr);
		if (ret < 0) {
			printf("ioctl IPU_ALLOC fail\n");
			goto err6;
		}
		ovbuf = mmap(0, ovsize, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd_ipu, t->overlay.paddr);
		if (!ovbuf) {
			printf("mmap fail\n");
			ret = -1;
			goto err7;
		}

		/*fill overlay buffer with dedicated data*/
		memset(ovbuf, 0x00, ovsize/4);
		memset(ovbuf+ovsize/4, 0x55, ovsize/4);
		memset(ovbuf+ovsize/2, 0xaa, ovsize/4);
		memset(ovbuf+ovsize*3/4, 0xff, ovsize/4);

		if (t->overlay.alpha.mode == IPU_ALPHA_MODE_LOCAL) {
			alpsize = t->overlay.alpha.loc_alp_paddr =
				t->overlay.width * t->overlay.height;
			ret = ioctl(fd_ipu, IPU_ALLOC, &t->overlay.alpha.loc_alp_paddr);
			if (ret < 0) {
				printf("ioctl IPU_ALLOC fail\n");
				goto err8;
			}
			alpbuf = mmap(0, alpsize, PROT_READ | PROT_WRITE,
					MAP_SHARED, fd_ipu, t->overlay.alpha.loc_alp_paddr);
			if (!alpbuf) {
				printf("mmap fail\n");
				ret = -1;
				goto err9;
			}

			/*fill loc alpha buffer with dedicated data*/
			memset(alpbuf, 0x00, alpsize/4);
			memset(alpbuf+alpsize/4, 0x55, alpsize/4);
			memset(alpbuf+alpsize/2, 0xaa, alpsize/4);
			memset(alpbuf+alpsize*3/4, 0xff, alpsize/4);
		}
	}

	if (test_handle.show_to_fb) {
		int found = 0, i;
		char fb_dev[] = "/dev/fb0";
		char fb_name[16];

		if (!strcmp(test_handle.outfile, "ipu0-1st-ovfb"))
			memcpy(fb_name, "DISP3 FG", 9);
		if (!strcmp(test_handle.outfile, "ipu0-2nd-fb"))
			memcpy(fb_name, "DISP3 BG - DI1", 15);
		if (!strcmp(test_handle.outfile, "ipu1-1st-ovfb"))
			memcpy(fb_name, "DISP4 FG", 9);
		if (!strcmp(test_handle.outfile, "ipu1-2nd-fb"))
			memcpy(fb_name, "DISP4 BG - DI1", 15);

		for (i=0; i<5; i++) {
			fb_dev[7] = '0';
			fb_dev[7] += i;
			fd_fb = open(fb_dev, O_RDWR, 0);
			if (fd_fb > 0) {
				ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix);
				if (!strcmp(fb_fix.id, fb_name)) {
					printf("found fb dev %s\n", fb_dev);
					found = 1;
					break;
				} else
					close(fd_fb);
			}
		}

		if (!found) {
			printf("can not find fb dev %s\n", fb_name);
			ret = -1;
			goto err10;
		}

		ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var);
		fb_var.xres = t->output.width;
		fb_var.xres_virtual = fb_var.xres;
		fb_var.yres = t->output.height;
		fb_var.yres_virtual = fb_var.yres;
		fb_var.activate |= FB_ACTIVATE_FORCE;
		fb_var.vmode |= FB_VMODE_YWRAP;
		fb_var.nonstd = t->output.format;
		fb_var.bits_per_pixel = fmt_to_bpp(t->output.format);

		ret = ioctl(fd_fb, FBIOPUT_VSCREENINFO, &fb_var);
		if (ret < 0) {
			printf("fb ioctl FBIOPUT_VSCREENINFO fail\n");
			goto err11;
		}
		ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var);
		ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix);

		outpaddr = fb_fix.smem_start;
		blank = FB_BLANK_UNBLANK;
		ioctl(fd_fb, FBIOBLANK, blank);
	} else {
		osize = t->output.paddr =
			t->output.width * t->output.height
			* fmt_to_bpp(t->output.format)/8;
		ret = ioctl(fd_ipu, IPU_ALLOC, &t->output.paddr);
		if (ret < 0) {
			printf("ioctl IPU_ALLOC fail\n");
			goto err10;
		}
		outbuf = mmap(0, osize, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd_ipu, t->output.paddr);
		if (!outbuf) {
			printf("mmap fail\n");
			ret = -1;
			goto err11;
		}

		file_out = fopen(test_handle.outfile, "wb");
		if (file_out == NULL) {
			printf("can not open output file %s\n", test_handle.outfile);
			ret = -1;
			goto err12;
		}
	}

again:
	ret = ioctl(fd_ipu, IPU_CHECK_TASK, t);
	if (ret != IPU_CHECK_OK) {
		if (ret > IPU_CHECK_ERR_MIN) {
			if (ret == IPU_CHECK_ERR_SPLIT_INPUTW_OVER) {
				t->input.crop.w -= 8;
				goto again;
			}
			if (ret == IPU_CHECK_ERR_SPLIT_INPUTH_OVER) {
				t->input.crop.h -= 8;
				goto again;
			}
			if (ret == IPU_CHECK_ERR_SPLIT_OUTPUTW_OVER) {
				t->output.crop.w -= 8;
				goto again;
			}
			if (ret == IPU_CHECK_ERR_SPLIT_OUTPUTH_OVER) {
				t->output.crop.h -= 8;
				goto again;
			}
			ret = 0;
			printf("ipu task check fail\n");
			goto err13;
		}
	}
	dump_ipu_task(t);

task_begin:
	if (t->input.deinterlace.enable &&
		(t->input.deinterlace.motion != HIGH_MOTION))
		if (fread(vdibuf, 1, isize, file_in) < isize) {
			ret = 0;
			printf("Can not read enough data from input file\n");
			goto err13;
		}

	while((done_cnt < test_handle.fcount) && (ctrl_c_rev == 0)) {
		gettimeofday(&begin, NULL);

		if (t->input.deinterlace.enable &&
			(t->input.deinterlace.motion != HIGH_MOTION)) {
			memcpy(inbuf, vdibuf, isize);
			ret = fread(vdibuf, 1, isize, file_in);
		} else
			ret = fread(inbuf, 1, isize, file_in);
		if (ret < isize) {
			ret = 0;
			printf("Can not read enough data from input file\n");
			break;
		}

		if (test_handle.show_to_fb)
			t->output.paddr = outpaddr;

		ret = ioctl(fd_ipu, IPU_QUEUE_TASK, t);
		if (ret < 0) {
			printf("ioct IPU_QUEUE_TASK fail\n");
			break;
		}

		if (test_handle.show_to_fb) {
			ret = ioctl(fd_fb, FBIOPAN_DISPLAY, &fb_var);
			if (ret < 0) {
				printf("fb ioct FBIOPAN_DISPLAY fail\n");
				break;
			}
		} else {
			ret = fwrite(outbuf, 1, osize, file_out);
			if (ret < osize) {
				ret = -1;
				printf("Can not write enough data into output file\n");
				break;
			}
		}
		done_cnt++;
		total_cnt++;

		gettimeofday(&end, NULL);
		sec = end.tv_sec - begin.tv_sec;
		usec = end.tv_usec - begin.tv_usec;

		if (usec < 0) {
			sec--;
			usec = usec + 1000000;
		}
		run_time += (sec * 1000000) + usec;
	}

	if (ret >= 0) {
		done_loop++;
		if ((done_loop < test_handle.loop_cnt) && (ctrl_c_rev == 0)) {
			done_cnt = 0;
			fseek(file_in, 0L, SEEK_SET);
			goto task_begin;
		}
	}

	printf("total frame count %d avg frame time %d us, fps %f\n",
			total_cnt, run_time/total_cnt, total_cnt/(run_time/1000000.0));

err13:
	if (fd_fb) {
		blank = FB_BLANK_POWERDOWN;
		ioctl(fd_fb, FBIOBLANK, blank);
	}
	if (file_out)
		fclose(file_out);
err12:
	if (outbuf)
		munmap(outbuf, osize);
err11:
	if (fd_fb)
		close(fd_fb);
	if (t->output.paddr)
		ioctl(fd_ipu, IPU_FREE, &t->output.paddr);
err10:
	if (alpbuf)
		munmap(alpbuf, alpsize);
err9:
	if (t->overlay.alpha.loc_alp_paddr)
		ioctl(fd_ipu, IPU_FREE, &t->overlay.alpha.loc_alp_paddr);
err8:
	if (ovbuf)
		munmap(ovbuf, ovsize);
err7:
	if (t->overlay.paddr)
		ioctl(fd_ipu, IPU_FREE, &t->overlay.paddr);
err6:
	if (vdibuf)
		munmap(vdibuf, isize);
err5:
	if (t->input.paddr_n)
		ioctl(fd_ipu, IPU_FREE, &t->input.paddr_n);
err4:
	if (inbuf)
		munmap(inbuf, isize);
err3:
	if (t->input.paddr)
		ioctl(fd_ipu, IPU_FREE, &t->input.paddr);
err2:
	if (fd_ipu)
		close(fd_ipu);
err1:
	if (file_in)
		fclose(file_in);
err0:
	return ret;
}
