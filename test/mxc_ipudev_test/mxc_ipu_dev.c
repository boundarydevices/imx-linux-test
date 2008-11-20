/*
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <fcntl.h>
#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev.h>
#include <asm/arch/ipu.h>

static ipu_channel_t ic_chan = MEM_PRP_VF_MEM;
static ipu_channel_t rot_chan = MEM_ROT_VF_MEM;
static int rot_out_eof_irq = IPU_IRQ_PRP_VF_ROT_OUT_EOF;
static int ic_out_eof_irq = IPU_IRQ_PRP_VF_OUT_EOF;
static int rot_in_eof_irq = IPU_IRQ_PRP_VF_ROT_IN_EOF;
static int ic_in_eof_irq = IPU_IRQ_PRP_IN_EOF;

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

int ipu_ic_task(int fd_ipu, FILE *file_in, FILE *file_out,
		ipu_ic_param *i_para, ipu_ic_param *o_para)
{
	ipu_mem_info i_minfo = {0};
	ipu_mem_info o_minfo = {0};
	ipu_mem_info r_minfo = {0};
	ipu_event_info einfo;
	pp_mode_t pp_mode = NULL_MODE;
	ipu_channel_params_t params;
	int irq = -1, ret = 0, tmp;
	void * inbuf_start;
	void * outbuf_start;

	if(o_para->rot >= IPU_ROTATE_HORIZ_FLIP){
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

	if (pp_mode == NULL_MODE){
		printf("Dont want do any IC processing!\n");
		ret =  -1;
		goto done;
	}

	/*allocate memory*/
	i_minfo.size = i_para->width/8*i_para->height*fmt_to_bpp(i_para->fmt);
	if (ret = ioctl(fd_ipu, IPU_ALOC_MEM, &i_minfo) < 0) {
		printf("Ioctl IPU_ALOC_MEM failed!\n");
		goto done;
	}
	inbuf_start = mmap (NULL, i_minfo.size,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			fd_ipu, i_minfo.paddr);
	if (inbuf_start == NULL) {
		printf("mmap failed!\n");
		ret = -1;
		goto done;
	}

	o_minfo.size = o_para->width/8*o_para->height*fmt_to_bpp(o_para->fmt);
	if (ret = ioctl(fd_ipu, IPU_ALOC_MEM, &o_minfo) < 0) {
		printf("Ioctl IPU_ALOC_MEM failed!\n");
		goto done;
	}
	outbuf_start = mmap (NULL, o_minfo.size,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			fd_ipu, o_minfo.paddr);
	if (outbuf_start == NULL) {
		printf("mmap failed!\n");
		ret = -1;
		goto done;
	}
	if(pp_mode == (ROT_MODE | IC_MODE)) {
		r_minfo.size = o_para->width/8*o_para->height*fmt_to_bpp(o_para->fmt);
		if (ret = ioctl(fd_ipu, IPU_ALOC_MEM, &r_minfo) < 0) {
			printf("Ioctl IPU_ALOC_MEM failed!\n");
			goto done;
		}
	}

	if (fread(inbuf_start, 1, i_minfo.size, file_in) < i_minfo.size) {
		ret = -1;
		printf("Can not read enough data from input file!\n");
		goto done;
	}

	/*setup irq*/
	if(pp_mode & ROT_MODE)
		irq = rot_out_eof_irq;
	else
		irq = ic_out_eof_irq;

	ipu_clear_irq(irq);
	ret = ipu_register_generic_isr(irq, NULL);
	if (ret < 0) {
		printf("Ioctl IPU_REGISTER_GENERIC_ISR failed!\n");
		goto done;
	}

	printf("Enabling:");

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
				i_minfo.paddr,
				0,
				0, 0);

		ipu_init_channel_buffer(ic_chan,
				IPU_OUTPUT_BUFFER,
				o_para->fmt,
				o_para->width,
				o_para->height,
				o_para->width*bytes_per_pixel(o_para->fmt),
				o_para->rot,
				o_minfo.paddr,
				0,
				0, 0);

		ipu_select_buffer(ic_chan, IPU_INPUT_BUFFER, 0);
		ipu_select_buffer(ic_chan, IPU_OUTPUT_BUFFER, 0);
		ipu_enable_channel(ic_chan);
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
				i_minfo.paddr,
				0,
				0, 0);

		if(o_para->rot >= IPU_ROTATE_90_RIGHT){
			/*output swap*/
			tmp = o_para->width;
			o_para->width = o_para->height;
			o_para->height = tmp;
		}

		ipu_init_channel_buffer(rot_chan,
				IPU_OUTPUT_BUFFER,
				o_para->fmt,
				o_para->width,
				o_para->height,
				o_para->width*bytes_per_pixel(o_para->fmt),
				IPU_ROTATE_NONE,
				o_minfo.paddr,
				0,
				0, 0);

		ipu_select_buffer(rot_chan, IPU_INPUT_BUFFER, 0);
		ipu_select_buffer(rot_chan, IPU_OUTPUT_BUFFER, 0);
		ipu_enable_channel(rot_chan);
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
				i_minfo.paddr,
				0,
				0, 0);

		ipu_init_channel_buffer(ic_chan,
				IPU_OUTPUT_BUFFER,
				o_para->fmt,
				o_para->width,
				o_para->height,
				o_para->width*bytes_per_pixel(o_para->fmt),
				IPU_ROTATE_NONE,
				r_minfo.paddr,
				0,
				0, 0);

		ipu_init_channel(rot_chan, NULL);

		ipu_init_channel_buffer(rot_chan,
				IPU_INPUT_BUFFER,
				o_para->fmt,
				o_para->width,
				o_para->height,
				o_para->width*bytes_per_pixel(o_para->fmt),
				o_para->rot,
				r_minfo.paddr,
				0,
				0, 0);

		if(o_para->rot >= IPU_ROTATE_90_RIGHT){
			/*output swap*/
			tmp = o_para->width;
			o_para->width = o_para->height;
			o_para->height = tmp;
		}

		ipu_init_channel_buffer(rot_chan,
				IPU_OUTPUT_BUFFER,
				o_para->fmt,
				o_para->width,
				o_para->height,
				o_para->width*bytes_per_pixel(o_para->fmt),
				IPU_ROTATE_NONE,
				o_minfo.paddr,
				0,
				0, 0);

		ipu_link_channels(ic_chan, rot_chan);

		ipu_select_buffer(rot_chan, IPU_OUTPUT_BUFFER, 0);
		ipu_enable_channel(rot_chan);

		ipu_select_buffer(ic_chan, IPU_INPUT_BUFFER, 0);
		ipu_select_buffer(ic_chan, IPU_OUTPUT_BUFFER, 0);
		ipu_enable_channel(ic_chan);
	}

	do {
		ipu_get_interrupt_event(&einfo);
	} while (einfo.irq != irq);
	ipu_clear_irq(irq);
	ipu_free_irq(irq, NULL);

	if (fwrite(outbuf_start, 1, o_minfo.size, file_out) < o_minfo.size) {
		ret = -1;
		printf("Can not write enough data into output file!\n");
	}

	if((pp_mode & ROT_MODE) && (pp_mode & IC_MODE)) {
		ipu_clear_irq(ic_out_eof_irq);
		ipu_unlink_channels(ic_chan, rot_chan);
	}

	if(pp_mode & IC_MODE){
		ipu_clear_irq(ic_in_eof_irq);
		ipu_disable_channel(ic_chan, 1);
		ipu_uninit_channel(ic_chan);
	}

	if(pp_mode & ROT_MODE){
		ipu_clear_irq(rot_in_eof_irq);
		ipu_disable_channel(rot_chan, 1);
		ipu_uninit_channel(rot_chan);
	}
done:

	if (i_minfo.vaddr) {
		if (inbuf_start)
			munmap(inbuf_start, i_minfo.size);
		ioctl(fd_ipu, IPU_FREE_MEM, &i_minfo);
	}
	if (o_minfo.vaddr) {
		if (outbuf_start)
			munmap(outbuf_start, o_minfo.size);
		ioctl(fd_ipu, IPU_FREE_MEM, &o_minfo);
	}
	if (r_minfo.vaddr)
		ioctl(fd_ipu, IPU_FREE_MEM, &r_minfo);

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
	}

	if ((i_para->width == 0) || (i_para->height == 0) ||
			(o_para->width == 0) || (o_para->height == 0))
		return -1;

	return 0;
}

int main(int argc, char *argv[])
{
	int fd_ipu;
	FILE *file_in, *file_out;
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
	file_out = fopen("out.bin", "wb");

	ipu_ic_task(fd_ipu, file_in, file_out,&i_para, &o_para);

	fclose(file_in);
	fclose(file_out);
	ipu_close();

	return 0;
}
