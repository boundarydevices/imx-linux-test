/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file mxc_ipu_dev.c
 *
 * @brief IPU device implementation
 *
 * @ingroup IPU
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev.h>
#include <linux/autoconf.h>
#include <linux/ipu.h>
#include <linux/mxcfb.h>

#ifdef CONFIG_MXC_IPU_V1
#define ROT_BEGIN	IPU_ROTATE_90_RIGHT
#else
#define ROT_BEGIN	IPU_ROTATE_HORIZ_FLIP
#endif

int ipu_get_interrupt_event(ipu_event_info *ev);

static ipu_channel_t ic_chan = MEM_PRP_VF_MEM;
static ipu_channel_t rot_chan = MEM_ROT_VF_MEM;
static ipu_channel_t fb_chan = MEM_BG_SYNC;
static ipu_channel_t begin_chan, end_chan;
static int rot_out_eof_irq = IPU_IRQ_PRP_VF_ROT_OUT_EOF;
static int ic_out_eof_irq = IPU_IRQ_PRP_VF_OUT_EOF;
static int rot_in_eof_irq = IPU_IRQ_PRP_VF_ROT_IN_EOF;
static int ic_in_eof_irq = IPU_IRQ_PRP_IN_EOF;
static int fcount = 1, irq = -1;
static char * outfile = NULL;
static void * fb_mem = NULL;
static int show_to_fb = 0, fb_stride = 0;
static int fd_fb, screen_size = 0;

typedef enum
{
	NULL_MODE = 0,
	IC_MODE = 0x1,
	ROT_MODE = 0x2
} pp_mode_t;

typedef enum {
	RGB_CS,
	YUV_CS,
	NULL_CS
} cs_t;

typedef struct{
	int width;
	int height;
	int rot;
	int fmt;
}ipu_ic_param;

static u32 fmt_to_bpp(u32 pixelformat)
{
	u32 bpp;

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
		case IPU_PIX_FMT_YUV420P2:
			bpp = 12;
			break;
		default:
			bpp = 8;
			break;
	}
	return bpp;
}

static cs_t colorspaceofpixel(int fmt)
{
	switch(fmt)
	{
		case IPU_PIX_FMT_RGB565:
		case IPU_PIX_FMT_BGR24:
		case IPU_PIX_FMT_RGB24:
		case IPU_PIX_FMT_BGRA32:
		case IPU_PIX_FMT_BGR32:
		case IPU_PIX_FMT_RGBA32:
		case IPU_PIX_FMT_RGB32:
		case IPU_PIX_FMT_ABGR32:
			return RGB_CS;
			break;
		case IPU_PIX_FMT_UYVY:
		case IPU_PIX_FMT_YUV420P2:
		case IPU_PIX_FMT_YUV420P:
		case IPU_PIX_FMT_YVU422P:
		case IPU_PIX_FMT_YUV422P:
		case IPU_PIX_FMT_YUV444:
			return YUV_CS;
			break;
		default:
			return NULL_CS;
	}
}

static int need_csc(int ifmt, int ofmt)
{
	cs_t ics,ocs;

	ics = colorspaceofpixel(ifmt);
	ocs = colorspaceofpixel(ofmt);

	if((ics == NULL_CS) || (ocs == NULL_CS)){
		printf("Color Space not recognized!\n");
		return -1;
	}else if(ics != ocs)
		return 1;

	return 0;
}

static pp_mode_t ipu_ic_task_check(ipu_ic_param *i_para, ipu_ic_param *o_para)
{
	pp_mode_t pp_mode = NULL_MODE;
	int tmp;

	if(o_para->rot >= ROT_BEGIN){
		if(o_para->rot >= IPU_ROTATE_90_RIGHT){
			/*output swap*/
			tmp = o_para->width;
			o_para->width = o_para->height;
			o_para->height = tmp;
		}
		pp_mode |= ROT_MODE;
	}

	/*need resize or CSC?*/
	if((i_para->width != o_para->width) ||
			(i_para->height != o_para->height) ||
			need_csc(i_para->fmt,o_para->fmt))
		pp_mode |= IC_MODE;

	/*need flip?*/
	if((pp_mode == NULL_MODE) && (o_para->rot > IPU_ROTATE_NONE ))
		pp_mode |= IC_MODE;

	/*need IDMAC do format(same color space)?*/
	if((pp_mode == NULL_MODE) && (i_para->fmt != o_para->fmt))
		pp_mode |= IC_MODE;

	return pp_mode;
}

static int ipu_ic_mem_alloc(ipu_ic_param *i_para, ipu_ic_param *o_para,
		ipu_mem_info i_minfo[], void * inbuf_start[],
		ipu_mem_info o_minfo[], void * outbuf_start[],
		ipu_mem_info r_minfo[], pp_mode_t pp_mode,
		int bufcnt, int fd_ipu)
{
	int i, ret = 0;

	for (i=0;i<bufcnt;i++) {
		i_minfo[i].size = i_para->width/8*i_para->height*fmt_to_bpp(i_para->fmt);
		if (ioctl(fd_ipu, IPU_ALOC_MEM, &(i_minfo[i])) < 0) {
			printf("Ioctl IPU_ALOC_MEM failed!\n");
			ret = -1;
			goto err;
		}
		inbuf_start[i] = mmap (NULL, i_minfo[i].size,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				fd_ipu, i_minfo[i].paddr);
		if (inbuf_start[i] == MAP_FAILED) {
			printf("mmap failed!\n");
			ret = -1;
			goto err;
		}

		if (show_to_fb == 0) {
			o_minfo[i].size = o_para->width/8*o_para->height*fmt_to_bpp(o_para->fmt);
			if (ioctl(fd_ipu, IPU_ALOC_MEM, &(o_minfo[i])) < 0) {
				printf("Ioctl IPU_ALOC_MEM failed!\n");
				ret = -1;
				goto err;
			}
			outbuf_start[i] = mmap (NULL, o_minfo[i].size,
					PROT_READ | PROT_WRITE, MAP_SHARED,
					fd_ipu, o_minfo[i].paddr);
			if (outbuf_start[i] == MAP_FAILED) {
				printf("mmap failed!\n");
				ret = -1;
				goto err;
			}
		}

		if(pp_mode == (ROT_MODE | IC_MODE)) {
			r_minfo[i].size = o_para->width/8*o_para->height*fmt_to_bpp(o_para->fmt);
			if (ioctl(fd_ipu, IPU_ALOC_MEM, &r_minfo[i]) < 0) {
				printf("Ioctl IPU_ALOC_MEM failed!\n");
				ret = -1;
				goto err;
			}
		}
	}

	/*for the case output direct to framebuffer*/
	if (show_to_fb) {
		int owidth, oheight;
        	struct fb_fix_screeninfo fb_fix;
        	struct fb_var_screeninfo fb_var;

                if ((fd_fb = open("/dev/fb0", O_RDWR, 0)) < 0)
                {
                        printf("Unable to open /dev/fb0\n");
                        ret = -1;
                        goto err;
                }

                if ( ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix) < 0) {
			printf("Get FB fix info failed!\n");
			close(fd_fb);
			ret = -1;
                        goto err;
                }
        	if ( ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var) < 0) {
			printf("Get FB var info failed!\n");
			close(fd_fb);
			ret = -1;
                        goto err;
                }

		if (pp_mode & ROT_MODE){
			owidth = o_para->height;
			oheight = o_para->width;
		} else {
			owidth = o_para->width;
			oheight = o_para->height;
		}

		if ((owidth > fb_var.xres) || (oheight > fb_var.yres)
			|| (colorspaceofpixel(o_para->fmt) != RGB_CS)
			|| (fmt_to_bpp(o_para->fmt) != fb_var.bits_per_pixel)) {
			printf("Output image is not fit for /dev/fb0!\n");
			close(fd_fb);
			ret = -1;
			goto err;
		}

		screen_size = fb_var.yres * fb_fix.line_length;
		o_minfo[0].paddr = fb_fix.smem_start + screen_size;

		fb_mem = mmap(0, 2*screen_size,
				PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
		if (fb_mem == MAP_FAILED) {
			printf("mmap failed!\n");
			ret = -1;
			goto err;
		}

		if (bufcnt > 1) {
			o_minfo[1].paddr = fb_fix.smem_start;
			/*make two buffer as the same origin to avoid flick*/
			memcpy(fb_mem + screen_size, fb_mem, screen_size);
		}
		fb_stride = fb_var.xres * fb_var.bits_per_pixel/8;

	}
err:
	return ret;
}

static void ipu_ic_mem_free(ipu_mem_info i_minfo[], void * inbuf_start[],
		ipu_mem_info o_minfo[], void * outbuf_start[],
		ipu_mem_info r_minfo[], int bufcnt, int fd_ipu)
{
	int i;

	for (i=0;i<bufcnt;i++) {
		if (i_minfo[i].vaddr) {
			if (inbuf_start[i])
				munmap(inbuf_start[i], i_minfo[i].size);
			ioctl(fd_ipu, IPU_FREE_MEM, &(i_minfo[i]));
		}
		if (show_to_fb == 0) {
			if (o_minfo[i].vaddr) {
				if (outbuf_start[i])
					munmap(outbuf_start[i], o_minfo[i].size);
				ioctl(fd_ipu, IPU_FREE_MEM, &(o_minfo[i]));
			}
		}
		if (r_minfo[i].vaddr)
			ioctl(fd_ipu, IPU_FREE_MEM, &(r_minfo[i]));
	}
	if (show_to_fb){
		if (fb_mem)
			munmap(fb_mem, 2*screen_size);
		close(fd_fb);
	}
}

static int ipu_ic_task_setup(ipu_ic_param *i_para, ipu_ic_param *o_para,
		ipu_mem_info i_minfo[], ipu_mem_info o_minfo[],
		ipu_mem_info r_minfo[], pp_mode_t pp_mode)
{
	ipu_channel_params_t params;
	int tmp, ret = 0, out_stride;


	printf("Enabling:");

	/*Setup ipu channel*/
	/*Only IC*/
	if(pp_mode == IC_MODE){
		printf("Only IC!\n");

		memset(&params, 0, sizeof (params));

		params.mem_prp_vf_mem.in_width = i_para->width;
		params.mem_prp_vf_mem.in_height = i_para->height;
		params.mem_prp_vf_mem.in_pixel_fmt = i_para->fmt;

		params.mem_prp_vf_mem.out_width = o_para->width;
		params.mem_prp_vf_mem.out_height = o_para->height;
		params.mem_prp_vf_mem.out_pixel_fmt = o_para->fmt;

		ipu_init_channel(ic_chan, &params);

		ipu_init_channel_buffer(ic_chan,
				IPU_INPUT_BUFFER,
				i_para->fmt,
				i_para->width,
				i_para->height,
				i_para->width*bytes_per_pixel(i_para->fmt),
				IPU_ROTATE_NONE,
				i_minfo[0].paddr,
				i_minfo[1].paddr,
				0, 0);

		if (show_to_fb)
			out_stride = fb_stride;
		else
			out_stride = o_para->width*bytes_per_pixel(o_para->fmt);

		ipu_init_channel_buffer(ic_chan,
				IPU_OUTPUT_BUFFER,
				o_para->fmt,
				o_para->width,
				o_para->height,
				out_stride,
				o_para->rot,
				o_minfo[0].paddr,
				o_minfo[1].paddr,
				0, 0);

		begin_chan = end_chan = ic_chan;
	}
	/*Only ROT*/
	else if (pp_mode == ROT_MODE){
		printf("Only ROT\n");

		ipu_init_channel(rot_chan, NULL);

		ipu_init_channel_buffer(rot_chan,
				IPU_INPUT_BUFFER,
				i_para->fmt,
				i_para->width,
				i_para->height,
				i_para->width*bytes_per_pixel(i_para->fmt),
				o_para->rot,
				i_minfo[0].paddr,
				i_minfo[1].paddr,
				0, 0);

		if(o_para->rot >= IPU_ROTATE_90_RIGHT){
			/*output swap*/
			tmp = o_para->width;
			o_para->width = o_para->height;
			o_para->height = tmp;
		}

		if (show_to_fb)
			out_stride = fb_stride;
		else
			out_stride = o_para->width*bytes_per_pixel(o_para->fmt);

		ipu_init_channel_buffer(rot_chan,
				IPU_OUTPUT_BUFFER,
				o_para->fmt,
				o_para->width,
				o_para->height,
				out_stride,
				IPU_ROTATE_NONE,
				o_minfo[0].paddr,
				o_minfo[1].paddr,
				0, 0);

		begin_chan = end_chan = rot_chan;
	}
	/*IC ROT*/
	else if(pp_mode == (IC_MODE | ROT_MODE)){
		printf("IC + ROT\n");

		memset(&params, 0, sizeof (params));

		params.mem_prp_vf_mem.in_width = i_para->width;
		params.mem_prp_vf_mem.in_height = i_para->height;
		params.mem_prp_vf_mem.in_pixel_fmt = i_para->fmt;

		params.mem_prp_vf_mem.out_width = o_para->width;
		params.mem_prp_vf_mem.out_height = o_para->height;
		params.mem_prp_vf_mem.out_pixel_fmt = o_para->fmt;

		ipu_init_channel(ic_chan, &params);

		ipu_init_channel_buffer(ic_chan,
				IPU_INPUT_BUFFER,
				i_para->fmt,
				i_para->width,
				i_para->height,
				i_para->width*bytes_per_pixel(i_para->fmt),
				IPU_ROTATE_NONE,
				i_minfo[0].paddr,
				i_minfo[1].paddr,
				0, 0);

		ipu_init_channel_buffer(ic_chan,
				IPU_OUTPUT_BUFFER,
				o_para->fmt,
				o_para->width,
				o_para->height,
				o_para->width*bytes_per_pixel(o_para->fmt),
				IPU_ROTATE_NONE,
				r_minfo[0].paddr,
				r_minfo[1].paddr,
				0, 0);

		ipu_init_channel(rot_chan, NULL);

		ipu_init_channel_buffer(rot_chan,
				IPU_INPUT_BUFFER,
				o_para->fmt,
				o_para->width,
				o_para->height,
				o_para->width*bytes_per_pixel(o_para->fmt),
				o_para->rot,
				r_minfo[0].paddr,
				r_minfo[1].paddr,
				0, 0);

		if(o_para->rot >= IPU_ROTATE_90_RIGHT){
			/*output swap*/
			tmp = o_para->width;
			o_para->width = o_para->height;
			o_para->height = tmp;
		}

		if (show_to_fb)
			out_stride = fb_stride;
		else
			out_stride = o_para->width*bytes_per_pixel(o_para->fmt);

		ipu_init_channel_buffer(rot_chan,
				IPU_OUTPUT_BUFFER,
				o_para->fmt,
				o_para->width,
				o_para->height,
				out_stride,
				IPU_ROTATE_NONE,
				o_minfo[0].paddr,
				o_minfo[1].paddr,
				0, 0);

		ipu_link_channels(ic_chan, rot_chan);

		begin_chan = ic_chan;
		end_chan = rot_chan;
	}

	if (show_to_fb)
		ipu_link_channels(end_chan, fb_chan);

	return ret;
}

static int ipu_ic_task_enable(pp_mode_t pp_mode, int bufcnt)
{
	int ret = 0;

	/*setup irq*/
	if(pp_mode & ROT_MODE)
		irq = rot_out_eof_irq;
	else
		irq = ic_out_eof_irq;

	if (show_to_fb) {
		if (pp_mode == ROT_MODE)
			irq = rot_in_eof_irq;
		else
			irq = ic_in_eof_irq;
	}

	ipu_clear_irq(irq);
	ret = ipu_register_generic_isr(irq, NULL);
	if (ret < 0) {
		printf("Ioctl IPU_REGISTER_GENERIC_ISR failed!\n");
		return ret;
	}

	if(pp_mode == IC_MODE){
		ipu_select_buffer(ic_chan, IPU_INPUT_BUFFER, 0);
		ipu_select_buffer(ic_chan, IPU_OUTPUT_BUFFER, 0);
		if (bufcnt == 2) {
			ipu_select_buffer(ic_chan, IPU_INPUT_BUFFER, 1);
			ipu_select_buffer(ic_chan, IPU_OUTPUT_BUFFER, 1);
		}
		ipu_enable_channel(ic_chan);
	} else if (pp_mode == ROT_MODE){
		ipu_select_buffer(rot_chan, IPU_INPUT_BUFFER, 0);
		ipu_select_buffer(rot_chan, IPU_OUTPUT_BUFFER, 0);
		if (bufcnt == 2) {
			ipu_select_buffer(rot_chan, IPU_INPUT_BUFFER, 1);
			ipu_select_buffer(rot_chan, IPU_OUTPUT_BUFFER, 1);
		}
		ipu_enable_channel(rot_chan);
	} else if(pp_mode == (IC_MODE | ROT_MODE)){
		ipu_select_buffer(rot_chan, IPU_OUTPUT_BUFFER, 0);
		if (bufcnt == 2)
			ipu_select_buffer(rot_chan, IPU_OUTPUT_BUFFER, 1);
		ipu_enable_channel(rot_chan);

		ipu_select_buffer(ic_chan, IPU_INPUT_BUFFER, 0);
		ipu_select_buffer(ic_chan, IPU_OUTPUT_BUFFER, 0);
		if (bufcnt == 2) {
			ipu_select_buffer(ic_chan, IPU_INPUT_BUFFER, 1);
			ipu_select_buffer(ic_chan, IPU_OUTPUT_BUFFER, 1);
		}
		ipu_enable_channel(ic_chan);
	}
	return ret;
}

static void ipu_ic_task_disable(pp_mode_t pp_mode)
{
	ipu_free_irq(irq, NULL);

	if (show_to_fb)
		ipu_unlink_channels(end_chan, fb_chan);

	if((pp_mode & ROT_MODE) && (pp_mode & IC_MODE))
		ipu_unlink_channels(ic_chan, rot_chan);

	if(pp_mode & IC_MODE){
		ipu_clear_irq(ic_in_eof_irq);
		ipu_clear_irq(ic_out_eof_irq);
		ipu_disable_channel(ic_chan, 1);
		ipu_uninit_channel(ic_chan);
	}

	if(pp_mode & ROT_MODE){
		ipu_clear_irq(rot_in_eof_irq);
		ipu_clear_irq(rot_out_eof_irq);
		ipu_disable_channel(rot_chan, 1);
		ipu_uninit_channel(rot_chan);
	}
}

static int ctrl_c_rev = 0;
void ctrl_c_handler(int signum, siginfo_t *info, void *myact)
{
	ctrl_c_rev = 1;
}

int ipu_ic_task(int fd_ipu, FILE *file_in, FILE *file_out,
		ipu_ic_param *i_para, ipu_ic_param *o_para, int fcount)
{
	/*memory info for input/output/rotation buffers*/
	ipu_mem_info i_minfo[2] = {{0}, {0}};
	ipu_mem_info o_minfo[2] = {{0}, {0}};
	ipu_mem_info r_minfo[2] = {{0}, {0}};
	/*input/output buffer addr for mmap*/
	void * inbuf_start[2] = {0};
	void * outbuf_start[2] = {0};
	/*task mode*/
	pp_mode_t pp_mode = NULL_MODE;
	ipu_event_info einfo;
	int ret = 0, i, bufcnt = 1;
	struct sigaction act;

	pp_mode = ipu_ic_task_check(i_para, o_para);
	if (pp_mode == NULL_MODE){
		printf("Dont want do any IC processing!\n");
		goto done;
	}

	/*for ctrl-c*/
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = ctrl_c_handler;

	if(sigaction(SIGINT, &act, NULL) < 0)
	{
		printf("install sigal error\n");
		goto done;
	}

	if (file_out == NULL) show_to_fb = 1;
	if (fcount > 1) bufcnt = 2;
	if (ipu_ic_mem_alloc(i_para, o_para, i_minfo, inbuf_start,
		o_minfo, outbuf_start, r_minfo, pp_mode, bufcnt, fd_ipu) < 0)
		goto done;

	if (ipu_ic_task_setup(i_para, o_para, i_minfo, o_minfo, r_minfo, pp_mode) < 0)
		goto done;

	/*prepare bufcnt input buffer*/
	for (i=0;i<bufcnt;i++) {
		if (fread(inbuf_start[i], 1, i_minfo[i].size, file_in) < i_minfo[i].size) {
			ret = -1;
			printf("Can not read enough data from input file!\n");
			goto done;
		}
	}

	if ((ret = ipu_ic_task_enable(pp_mode, bufcnt)) < 0)
		goto done;

	/*loop to finish fcount frame*/
	for(i=0;i<fcount;i++) {
		static int pingpang = 0;

		/*wait for IPU task finish*/
		einfo.irq = 0;
		do {
			ipu_get_interrupt_event(&einfo);
		} while ((einfo.irq != irq) && !ctrl_c_rev);

		if (!show_to_fb)
			if(fwrite(outbuf_start[pingpang], 1, o_minfo[pingpang].size, file_out) < o_minfo[pingpang].size) {
				ret = -1;
				printf("Can not write enough data into output file!\n");
			}

		/*need fill in more data?*/
		if ((fcount > 2) && (i < (fcount-2))) {
			if (fread(inbuf_start[pingpang], 1, i_minfo[pingpang].size, file_in) < i_minfo[pingpang].size) {
				ret = -1;
				printf("Can not read enough data from input file!\n");
				break;
			}
			if (!show_to_fb)
				ipu_select_buffer(end_chan, IPU_OUTPUT_BUFFER, pingpang);
			ipu_select_buffer(begin_chan, IPU_INPUT_BUFFER, pingpang);
		}
		pingpang = (pingpang == 0) ? 1 : 0;

		if (ctrl_c_rev) {
			if (einfo.irq == irq)
				i++;
			break;
		}
	}

	printf("Frame %d done!\n",i);

	ipu_ic_task_disable(pp_mode);

	/*make sure buffer1 still at fbmem base*/
	if (show_to_fb && (i % 2)) {
		memcpy(fb_mem, fb_mem + screen_size, screen_size);
		ipu_select_buffer(fb_chan, IPU_INPUT_BUFFER, 1);
	}

done:
	ipu_ic_mem_free(i_minfo, inbuf_start, o_minfo, outbuf_start, r_minfo, bufcnt, fd_ipu);

	return ret;
}

int process_cmdline(int argc, char **argv, ipu_ic_param *i_para, ipu_ic_param *o_para)
{
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-iw") == 0) {
			i_para->width = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-ih") == 0) {
			i_para->height = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-ow") == 0) {
			o_para->width = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-oh") == 0) {
			o_para->height = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-r") == 0) {
			o_para->rot = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-c") == 0) {
			fcount = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-t") == 0) {
			i++;
			if (strcmp(argv[i], "PP") == 0)	{
				printf("Using PP task:\n");
				ic_chan = MEM_PP_MEM;
				rot_chan = MEM_ROT_PP_MEM;
				rot_out_eof_irq = IPU_IRQ_PP_ROT_OUT_EOF;
				ic_out_eof_irq = IPU_IRQ_PP_OUT_EOF;
				rot_in_eof_irq = IPU_IRQ_PP_ROT_IN_EOF;
				ic_in_eof_irq = IPU_IRQ_PP_IN_EOF;
			} else if (strcmp(argv[i], "ENC") == 0)	{
				printf("Using PRP ENC task:\n");
				ic_chan = MEM_PRP_ENC_MEM;
				rot_chan = MEM_ROT_ENC_MEM;
				rot_out_eof_irq = IPU_IRQ_PRP_ENC_ROT_OUT_EOF;
				ic_out_eof_irq = IPU_IRQ_PRP_ENC_OUT_EOF;
				rot_in_eof_irq = IPU_IRQ_PRP_ENC_ROT_IN_EOF;
				ic_in_eof_irq = IPU_IRQ_PRP_IN_EOF;
			} else if (strcmp(argv[i], "VF") == 0)	{
				printf("Using PRP VF task:\n");
				ic_chan = MEM_PRP_VF_MEM;
				rot_chan = MEM_ROT_VF_MEM;
				rot_out_eof_irq = IPU_IRQ_PRP_VF_ROT_OUT_EOF;
				ic_out_eof_irq = IPU_IRQ_PRP_VF_OUT_EOF;
				rot_in_eof_irq = IPU_IRQ_PRP_VF_ROT_IN_EOF;
				ic_in_eof_irq = IPU_IRQ_PRP_IN_EOF;
			}
		}
		else if (strcmp(argv[i], "-if") == 0) {
			i++;
			i_para->fmt = v4l2_fourcc(argv[i][0], argv[i][1],argv[i][2],argv[i][3]);
		}
		else if (strcmp(argv[i], "-of") == 0) {
			i++;
			o_para->fmt = v4l2_fourcc(argv[i][0], argv[i][1],argv[i][2],argv[i][3]);
		}
		else if (strcmp(argv[i], "-out") == 0) {
			outfile = argv[++i];
		}
	}

	if ((i_para->width == 0) || (i_para->height == 0) ||
			(o_para->width == 0) || (o_para->height == 0)
			|| (fcount < 1))
		return -1;

	return 0;
}

int main(int argc, char *argv[])
{
	int fd_ipu;
	FILE *file_in = NULL, *file_out = NULL;
	ipu_ic_param i_para = {0}, o_para = {0};

	i_para.fmt = v4l2_fourcc('R','G','B','P');
	o_para.fmt = v4l2_fourcc('R','G','B','P');

	if (process_cmdline(argc, argv, &i_para, &o_para) < 0) {
		printf("\nMXC IPU device IC process Test\n\n" \
				"Usage: mxc_ipu_dev.out\n" \
				"-iw <input width>\n" \
				"-ih <input height>\n" \
				"-if <input fourcc format>\n" \
				"-ow <output width>\n" \
				"-oh <output height>\n" \
				"-of <output fourcc format>\n" \
				"-r <output rotation>\n" \
				"-t <IC task:PP,ENC,VF>\n" \
				"-c <frame count>\n" \
				"-out <output file name>\n" \
				"<input raw file>\n\n" \
			"fourcc ref:\n" \
				"RGB565->RGBP\n" \
				"BGR24 ->BGR3\n" \
				"RGB24 ->RGB3\n" \
				"BGR32 ->BGR4\n" \
				"BGRA32->BGRA\n" \
				"RGB32 ->RGB4\n" \
				"RGBA32->RGBA\n" \
				"ABGR32->ABGR\n" \
				"YUYV  ->YUYV\n" \
				"UYVY  ->UYVY\n" \
				"YUV444->Y444\n" \
				"NV12  ->NV12\n" \
				"YUV420P->I420\n" \
				"YUV422P->422P\n" \
				"YVU422P->YV16\n\n");
		return -1;
	}

	fd_ipu = ipu_open();
	if (fd_ipu < 0)
		return -1;

	file_in = fopen(argv[argc-1], "rb");
	if (file_in == NULL){
		printf("there is no such file for reading %s\n", argv[argc-1]);
		ipu_close();
		return -1;
	}

	if (outfile)
		file_out = fopen(outfile, "wb");

	ipu_ic_task(fd_ipu, file_in, file_out, &i_para, &o_para, fcount);

	fclose(file_in);
	if (file_out)
		fclose(file_out);
	ipu_close();

	return 0;
}
