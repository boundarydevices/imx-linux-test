/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Copyright (c) 2006, Chips & Media.  All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vpu_test.h"

struct encode *enc;
struct decode *dec;
extern int quitflag;
extern struct capture_testbuffer cap_buffers[];
static int disp_clr_index = -1;
static int frame_id = 0;

static int
decode(void)
{
	DecHandle handle = dec->handle;
	DecParam decparam = {0};
	DecOutputInfo outinfo = {0};
	struct vpu_display *disp = dec->disp;
	RetCode ret;

	/* Suggest to enable prescan in loopback, then decoder performs scanning stream buffers
	 * to check whether data is enough to prevent decoder hang.
	 */
	decparam.prescanEnable = dec->cmdl->prescan;

	ret = vpu_DecStartOneFrame(handle, &decparam);
	if (ret != RETCODE_SUCCESS) {
		err_msg("DecStartOneFrame failed\n");
		return -1;
	}

	while (vpu_IsBusy()) {
		vpu_WaitForInt(200);
	}

	ret = vpu_DecGetOutputInfo(handle, &outinfo);
	if (ret == RETCODE_FAILURE) {
		goto out;
	} else if (ret != RETCODE_SUCCESS) {
		err_msg("vpu_DecGetOutputInfo failed %d\n", ret);
		return -1;
	}

	if (outinfo.notSufficientPsBuffer) {
		err_msg("PS Buffer overflow\n");
		return -1;
	}

	if (outinfo.notSufficientSliceBuffer) {
		err_msg("Slice Buffer overflow\n");
		return -1;
	}

	if ((outinfo.indexFrameDisplay == -1) ||
			(outinfo.indexFrameDisplay > dec->fbcount))
		return -1;

	if ((outinfo.prescanresult == 0) && (decparam.dispReorderBuf == 0)) {
		warn_msg("Prescan Enable: not enough bs data\n");
		goto out;
	}

	if ((outinfo.indexFrameDisplay == -3) ||
	    (outinfo.indexFrameDisplay == -2)) {
		if (disp_clr_index >= 0) {
			ret = vpu_DecClrDispFlag(handle, disp_clr_index);
			if (ret)
				err_msg("vpu_DecClrDispFlag failed Error code"
				    " %d\n", ret);
		}
		disp_clr_index = outinfo.indexFrameDisplay;
		goto out;
	}

	ret = v4l_put_data(disp, outinfo.indexFrameDisplay, V4L2_FIELD_ANY);
	if (ret)
		return -1;

	if (disp_clr_index >= 0) {
		ret = vpu_DecClrDispFlag(handle, disp_clr_index);
		if (ret)
			err_msg("vpu_DecClrDispFlag failed Error code"
			     " %d\n", ret);
	}
	disp_clr_index = disp->buf.index;

out:
	return 0;
}

static int
dec_fill_bsbuffer(char *buf, int size)
{
	DecHandle handle = dec->handle;
	u32 bs_va_startaddr = dec->virt_bsbuf_addr;
	u32 bs_va_endaddr = bs_va_startaddr + STREAM_BUF_SIZE;
	u32 bs_pa_startaddr = dec->phy_bsbuf_addr;
	PhysicalAddress pa_read_ptr, pa_write_ptr;
	u32 target_addr, space;
	RetCode ret;
	int room;

	ret = vpu_DecGetBitstreamBuffer(handle, &pa_read_ptr, &pa_write_ptr,
					&space);
	if (ret != RETCODE_SUCCESS) {
		err_msg("vpu_DecGetBitstreamBuffer failed\n");
		return -1;
	}

	/* Decoder bitstream buffer is empty */
	if ((space <= 0) || (size <= 0))
		return 0;

	if (space < size)
		return 0;

	/* Fill the bitstream buffer */
	target_addr = bs_va_startaddr + (pa_write_ptr - bs_pa_startaddr);
	if ( (target_addr + size) > bs_va_endaddr) {
		room = bs_va_endaddr - target_addr;
		memcpy((u8 *)target_addr, buf, room);
		memcpy((u8 *)bs_va_startaddr, (buf + room), (size - room));
	} else {
		memcpy((u8 *)target_addr, buf, size);
	}

	ret = vpu_DecUpdateBitstreamBuffer(handle, size);
	if (ret != RETCODE_SUCCESS) {
		err_msg("vpu_DecUpdateBitstreamBuffer failed\n");
		return -1;
	}

	return size;
}

static int
encoder_fill_headers(void)
{
	EncHeaderParam enchdr_param = {0};
	EncHandle handle = enc->handle;
	RetCode ret;

	u32 vbuf;
	u32 phy_bsbuf  = enc->phy_bsbuf_addr;
	u32 virt_bsbuf = enc->virt_bsbuf_addr;

	/* Must put encode header before encoding */
	if (enc->cmdl->format == STD_MPEG4) {
		if (!cpu_is_mx51()) {
			enchdr_param.headerType = VOS_HEADER;
			vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &enchdr_param);

			vbuf = (virt_bsbuf + enchdr_param.buf - phy_bsbuf);
			ret = dec_fill_bsbuffer((void *)vbuf, enchdr_param.size);
			if (ret < 0)
				return -1;

			enchdr_param.headerType = VIS_HEADER;
			vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &enchdr_param);

			vbuf = (virt_bsbuf + enchdr_param.buf - phy_bsbuf);
			ret = dec_fill_bsbuffer((void *)vbuf, enchdr_param.size);
			if (ret < 0)
				return -1;
		}

		enchdr_param.headerType = VOL_HEADER;
		vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &enchdr_param);

		vbuf = (virt_bsbuf + enchdr_param.buf - phy_bsbuf);
		ret = dec_fill_bsbuffer((void *)vbuf, enchdr_param.size);
		if (ret < 0)
			return -1;
	} else if (enc->cmdl->format == STD_AVC) {
		enchdr_param.headerType = SPS_RBSP;
		vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);

		vbuf = (virt_bsbuf + enchdr_param.buf - phy_bsbuf);
		ret = dec_fill_bsbuffer((void *)vbuf, enchdr_param.size);
		if (ret < 0)
			return -1;
		enchdr_param.headerType = PPS_RBSP;
		vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);

		vbuf = (virt_bsbuf + enchdr_param.buf - phy_bsbuf);
		ret = dec_fill_bsbuffer((void *)vbuf, enchdr_param.size);
		if (ret < 0)
			return -1;
	} else if (enc->cmdl->format == STD_MJPG) {
		if (enc->huffTable)
			free(enc->huffTable);
		if (enc->qMatTable)
			free(enc->qMatTable);
	}

	return 0;
}

static int
encode(void)
{
	EncHandle handle = enc->handle;
	EncParam  enc_param = {0};
	EncOutputInfo outinfo = {0};
	RetCode ret;
	int src_fbid = enc->src_fbid, img_size;
	FrameBuffer *fb = enc->fb;
	struct v4l2_buffer v4l2_buf;
	u32 vbuf;

	ret = v4l_get_capture_data(&v4l2_buf);
	if (ret < 0) {
		return -1;
	}

	img_size = enc->picwidth * enc->picheight;
	fb[src_fbid].bufY = cap_buffers[v4l2_buf.index].offset;
	fb[src_fbid].bufCb = fb[src_fbid].bufY + img_size;
	fb[src_fbid].bufCr = fb[src_fbid].bufCb + (img_size >> 2);

	enc_param.sourceFrame = &enc->fb[src_fbid];
	enc_param.quantParam = 30;
	enc_param.skipPicture = 0;

	if ((frame_id % 10) == 0)
		enc_param.forceIPicture = 1;
	else
		enc_param.forceIPicture = 0;

	ret = vpu_EncStartOneFrame(handle, &enc_param);
	if (ret != RETCODE_SUCCESS) {
		err_msg("EncStartOneFrame failed\n");
		return -1;
	}

	while (vpu_IsBusy()) {
		vpu_WaitForInt(200);
	}

	ret = vpu_EncGetOutputInfo(handle, &outinfo);
	if (ret != RETCODE_SUCCESS) {
		err_msg("EncGetOutputInfo failed\n");
		return -1;
	}

	frame_id++;
	v4l_put_capture_data(&v4l2_buf);
	vbuf = (enc->virt_bsbuf_addr + outinfo.bitstreamBuffer
				- enc->phy_bsbuf_addr);
	ret = dec_fill_bsbuffer((void *)vbuf, outinfo.bitstreamSize);
	if (ret < 0) {
		err_msg("writing bitstream buffer failed\n");
		return -1;
	}

	if (ret != outinfo.bitstreamSize) {
		err_msg("Oops...\n");
	}

	return 0;
}

int
encdec_test(void *arg)
{
	struct cmd_line *cmdl = (struct cmd_line *)arg;
	vpu_mem_desc	enc_mem_desc = {0};
	vpu_mem_desc	dec_mem_desc = {0};
	vpu_mem_desc	ps_mem_desc = {0};
	vpu_mem_desc	slice_mem_desc = {0};
	int ret, i;

#if STREAM_ENC_PIC_RESET == 0
	return 0;
#endif

	/* allocate memory for must remember stuff */
	enc = (struct encode *)calloc(1, sizeof(struct encode));
	if (enc == NULL) {
		err_msg("Failed to allocate encode structure\n");
		return -1;
	}

	/* get physical contigous bit stream buffer */
	enc_mem_desc.size = STREAM_BUF_SIZE;
	ret = IOGetPhyMem(&enc_mem_desc);
	if (ret) {
		err_msg("Unable to obtain physical memory\n");
		free(enc);
		return -1;
	}

	/* mmap that physical buffer */
	enc->virt_bsbuf_addr = IOGetVirtMem(&enc_mem_desc);
	if (enc->virt_bsbuf_addr <= 0) {
		err_msg("Unable to map physical memory\n");
		IOFreePhyMem(&enc_mem_desc);
		free(enc);
		return -1;
	}

	enc->phy_bsbuf_addr = enc_mem_desc.phy_addr;
	enc->cmdl = cmdl;
	enc->picwidth = 176;
	enc->picheight = 144;

	dec = (struct decode *)calloc(1, sizeof(struct decode));
	if (dec == NULL) {
		err_msg("Failed to allocate decode structure\n");
		goto err;
	}

	dec_mem_desc.size = STREAM_BUF_SIZE;
	ret = IOGetPhyMem(&dec_mem_desc);
	if (ret) {
		err_msg("Unable to obtain physical mem\n");
		free(dec);
		goto err;
	}

	dec->virt_bsbuf_addr = IOGetVirtMem(&dec_mem_desc);
	if (dec->virt_bsbuf_addr <= 0) {
		err_msg("Unable to obtain virtual mem\n");
		IOFreePhyMem(&dec_mem_desc);
		free(dec);
		goto err;
	}

	dec->phy_bsbuf_addr = dec_mem_desc.phy_addr;
	dec->cmdl = cmdl;

	if (cmdl->format == STD_AVC) {
		ps_mem_desc.size = PS_SAVE_SIZE;
		ret = IOGetPhyMem(&ps_mem_desc);
		if (ret) {
			err_msg("Unable to obtain physical ps save mem\n");
			goto err;
		}

		dec->phy_ps_buf = ps_mem_desc.phy_addr;
	}

	/* open the encoder */
	ret = encoder_open(enc);
	if (ret)
		goto err1;

	/* configure the encoder */
	ret = encoder_configure(enc);
	if (ret)
		goto err2;

	/* allocate memory for the frame buffers */
	ret = encoder_allocate_framebuffer(enc);
	if (ret)
		goto err2;

	dec->reorderEnable = 1;

	/* open decoder */
	ret = decoder_open(dec);
	if (ret)
		goto err3;

	ret = encoder_fill_headers();
	if (ret) {
		err_msg("fill headers failed\n");
		goto err4;
	}

	/* start capture */
	ret = v4l_start_capturing();
	if (ret < 0) {
		err_msg("v4l2 start failed\n");
		goto err4;
	}

	/* encode 10 frames first */
	for (i = 0; i < 10; i++) {
		ret = encode();
		if (ret)
			goto err5;
	}

	/* parse the bitstream */
	ret = decoder_parse(dec);
	if (ret) {
		err_msg("decoder parse failed\n");
		goto err5;
	}

	/* allocate slice buf */
	if (cmdl->format == STD_AVC) {
		slice_mem_desc.size = dec->phy_slicebuf_size;
		ret = IOGetPhyMem(&slice_mem_desc);
		if (ret) {
			err_msg("Unable to obtain physical slice save mem\n");
			goto err5;
		}

		dec->phy_slice_buf = slice_mem_desc.phy_addr;
	}

	/* allocate frame buffers */
	ret = decoder_allocate_framebuffer(dec);
	if (ret)
		goto err5;

	/* main encode decode loop */
	while (1) {
		ret = encode();
		if (ret) {
			err_msg("Encode failed\n");
			break;
		}

		ret = decode();
		if (ret) {
			err_msg("Decode failed\n");
			break;
		}

		if (quitflag)
			break;

	}

err5:
	v4l_stop_capturing();
err4:
	decoder_close(dec);

	/* free the frame buffers */
	decoder_free_framebuffer(dec);
err3:
	/* free the allocated framebuffers */
	encoder_free_framebuffer(enc);
err2:
	/* close the encoder */
	encoder_close(enc);
err1:
	if (cmdl->format == STD_AVC) {
		IOFreePhyMem(&ps_mem_desc);
		IOFreePhyMem(&slice_mem_desc);
	}

	IOFreeVirtMem(&dec_mem_desc);
	IOFreePhyMem(&dec_mem_desc);
	free(dec);
err:
	/* free the physical memory */
	IOFreeVirtMem(&enc_mem_desc);
	IOFreePhyMem(&enc_mem_desc);
	free(enc);
	return 0;
}

