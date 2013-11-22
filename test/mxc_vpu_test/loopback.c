/*
 * Copyright 2004-2013 Freescale Semiconductor, Inc.
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
static int rotid;

static int
decode(void)
{
	DecHandle handle = dec->handle;
	DecParam decparam = {0};
	DecOutputInfo outinfo = {0};
	struct vpu_display *disp = dec->disp;
	struct v4l_specific_data *v4l_rsd = (struct v4l_specific_data *)disp->render_specific_data;
	RetCode ret;
	int loop_id = 0, rot_stride = 0;

	/* Suggest to enable prescan in loopback, then decoder performs scanning stream buffers
	 * to check whether data is enough to prevent decoder hang.
	 */
	if (!cpu_is_mx6x())
		decparam.prescanEnable = dec->cmdl->prescan;

	if (dec->cmdl->format == STD_MJPG) {
		rot_stride = (dec->picwidth + 15) & ~15;
		vpu_DecGiveCommand(handle, SET_ROTATOR_STRIDE, &rot_stride);
		vpu_DecGiveCommand(handle, SET_ROTATOR_OUTPUT, (void *)&dec->fb[rotid]);
	}

	ret = vpu_DecStartOneFrame(handle, &decparam);
	if (ret == RETCODE_JPEG_BIT_EMPTY)
		goto out;
	else if (ret != RETCODE_SUCCESS) {
		err_msg("DecStartOneFrame failed, ret=%d\n", ret);
		return -1;
	}

	loop_id = 0;
	while (vpu_IsBusy()) {
		vpu_WaitForInt(200);
		if (loop_id == 10) {
			ret = vpu_SWReset(handle, 0);
			warn_msg("vpu_SWReset in dec\n");
			return -1;
		}
		loop_id ++;
	}

	ret = vpu_DecGetOutputInfo(handle, &outinfo);
	if (ret == RETCODE_FAILURE) {
		goto out;
	} else if (ret != RETCODE_SUCCESS) {
		err_msg("vpu_DecGetOutputInfo failed %d\n", ret);
		return -1;
	}

	if (cpu_is_mx6x() && (outinfo.decodingSuccess & 0x10)) {
		goto out;
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
			(outinfo.indexFrameDisplay > dec->regfbcount))
		return -1;

	if (!cpu_is_mx6x() && (outinfo.prescanresult == 0) && (decparam.dispReorderBuf == 0)) {
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

	if ((dec->cmdl->format == STD_MJPG) && (outinfo.indexFrameDisplay == 0))
		outinfo.indexFrameDisplay = rotid;

	ret = v4l_put_data(dec, outinfo.indexFrameDisplay, V4L2_FIELD_ANY, 30);
	if (ret) {
		err_msg("V4L put data failed, ret=%d\n", ret);
		return -1;
	}

	if (disp_clr_index >= 0) {
		ret = vpu_DecClrDispFlag(handle, disp_clr_index);
		if (ret)
			err_msg("vpu_DecClrDispFlag failed Error code"
			     " %d\n", ret);
	}
	disp_clr_index = v4l_rsd->buf.index;

	if (dec->cmdl->format == STD_MJPG) {
		rotid++;
		rotid %= dec->regfbcount;
	}

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
					(Uint32 *)&space);
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
		if (cpu_is_mx27()) {
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
		if (cpu_is_mx6x()) {
			EncParamSet enchdr_param = {0};
			enchdr_param.size = STREAM_BUF_SIZE;
			enchdr_param.pParaSet = malloc(STREAM_BUF_SIZE);
			if (enchdr_param.pParaSet) {
				vpu_EncGiveCommand(handle,ENC_GET_JPEG_HEADER, &enchdr_param);
				ret = dec_fill_bsbuffer((void *)enchdr_param.pParaSet, enchdr_param.size);
				free(enchdr_param.pParaSet);
				if (ret < 0)
					return -1;
			} else {
				err_msg("memory allocate failure\n");
				return -1;
			}
		}
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
	int src_fbid = enc->src_fbid, img_size, loop_id = 0;
	FrameBuffer *fb = enc->fb;
	struct v4l2_buffer v4l2_buf;
	u32 vbuf;

	if (cpu_is_mx6x() && (dec->cmdl->format == STD_MJPG)) {
		ret = encoder_fill_headers();
		if (ret) {
			err_msg("fill headers failed\n");
			return -1;
		}
	}

	ret = v4l_get_capture_data(&v4l2_buf);
	if (ret < 0) {
		return -1;
	}

	img_size = enc->src_picwidth * enc->src_picheight;
	fb[src_fbid].myIndex = enc->src_fbid + v4l2_buf.index;
	fb[src_fbid].bufY = cap_buffers[v4l2_buf.index].offset;
	fb[src_fbid].bufCb = fb[src_fbid].bufY + img_size;
	fb[src_fbid].bufCr = fb[src_fbid].bufCb + (img_size >> 2);
	fb[src_fbid].strideY = enc->src_picwidth;
	fb[src_fbid].strideC = enc->src_picwidth / 2;

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

	loop_id = 0;
	while (vpu_IsBusy()) {
		vpu_WaitForInt(200);
		if (loop_id == 10) {
			ret = vpu_SWReset(handle, 0);
			warn_msg("vpu_SWReset in enc\n");
			return -1;
		}
		loop_id ++;
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
		err_msg("Oops..., ret=%d, bsSize=%d\n", (int)ret, (int)outinfo.bitstreamSize);
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
	vpu_mem_desc	enc_scratch_mem_desc = {0};
	int ret, i;

#ifndef COMMON_INIT
	vpu_versioninfo ver;
	ret = vpu_Init(NULL);
	if (ret) {
		err_msg("VPU Init Failure.\n");
		return -1;
	}

	ret = vpu_GetVersionInfo(&ver);
	if (ret) {
		err_msg("Cannot get version info, err:%d\n", ret);
		vpu_UnInit();
		return -1;
	}

	info_msg("VPU firmware version: %d.%d.%d_r%d\n", ver.fw_major, ver.fw_minor,
						ver.fw_release, ver.fw_code);
	info_msg("VPU library version: %d.%d.%d\n", ver.lib_major, ver.lib_minor,
						ver.lib_release);
#endif

	/* allocate memory for must remember stuff */
	enc = (struct encode *)calloc(1, sizeof(struct encode));
	if (enc == NULL) {
		err_msg("Failed to allocate encode structure\n");
		ret = -1;
		goto err;
	}

	/* get physical contigous bit stream buffer */
	enc_mem_desc.size = STREAM_BUF_SIZE;
	ret = IOGetPhyMem(&enc_mem_desc);
	if (ret) {
		err_msg("Unable to obtain physical memory\n");
		goto err;
	}

	/* mmap that physical buffer */
	enc->virt_bsbuf_addr = IOGetVirtMem(&enc_mem_desc);
	if (enc->virt_bsbuf_addr <= 0) {
		err_msg("Unable to map physical memory\n");
		ret = -1;
		goto err;
	}

	enc->phy_bsbuf_addr = enc_mem_desc.phy_addr;
	enc->cmdl = cmdl;
	enc->src_picwidth = 176;
	enc->src_picheight = 144;

	dec = (struct decode *)calloc(1, sizeof(struct decode));
	if (dec == NULL) {
		err_msg("Failed to allocate decode structure\n");
		ret = -1;
		goto err;
	}

	dec_mem_desc.size = STREAM_BUF_SIZE;
	ret = IOGetPhyMem(&dec_mem_desc);
	if (ret) {
		err_msg("Unable to obtain physical mem\n");
		goto err1;
	}

	dec->virt_bsbuf_addr = IOGetVirtMem(&dec_mem_desc);
	if (dec->virt_bsbuf_addr <= 0) {
		err_msg("Unable to obtain virtual mem\n");
		ret = -1;
		goto err1;
	}

	dec->phy_bsbuf_addr = dec_mem_desc.phy_addr;
	dec->cmdl = cmdl;

	if (cmdl->format == STD_AVC) {
		ps_mem_desc.size = PS_SAVE_SIZE;
		ret = IOGetPhyMem(&ps_mem_desc);
		if (ret) {
			err_msg("Unable to obtain physical ps save mem\n");
			goto err1;
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

	/* allocate scratch buf */
	if (cpu_is_mx6x() && (cmdl->format == STD_MPEG4) && enc->mp4_dataPartitionEnable) {
		enc_scratch_mem_desc.size = MPEG4_SCRATCH_SIZE;
		ret = IOGetPhyMem(&enc_scratch_mem_desc);
		if (ret) {
			err_msg("Unable to obtain physical slice save mem\n");
			goto err2;
		}
		enc->scratchBuf.bufferBase = enc_scratch_mem_desc.phy_addr;
		enc->scratchBuf.bufferSize = enc_scratch_mem_desc.size;
	}

	/* allocate memory for the frame buffers */
	ret = encoder_allocate_framebuffer(enc);
	if (ret)
		goto err2;

	dec->reorderEnable = 1;

	/* open decoder */
	ret = decoder_open(dec);
	if (ret)
		goto err2;


	/* For MX6 MJPG, header must be inserted before each frame, only need to
	 * fill header here once for other codecs */
	if (!(cpu_is_mx6x() && (enc->cmdl->format == STD_MJPG))) {
		ret = encoder_fill_headers();
		if (ret) {
			err_msg("fill headers failed\n");
			goto err3;
		}
	}

	/* start capture */
	ret = v4l_start_capturing();
	if (ret < 0) {
		err_msg("v4l2 start failed\n");
		goto err3;
	}

	/* encode 5 frames first */
	for (i = 0; i < 5; i++) {
		ret = encode();
		if (ret)
			goto err4;
	}

	/* parse the bitstream */
	ret = decoder_parse(dec);
	if (ret) {
		err_msg("decoder parse failed\n");
		goto err4;
	}

	/* allocate slice buf */
	if (cmdl->format == STD_AVC) {
		slice_mem_desc.size = dec->phy_slicebuf_size;
		ret = IOGetPhyMem(&slice_mem_desc);
		if (ret) {
			err_msg("Unable to obtain physical slice save mem\n");
			goto err4;
		}

		dec->phy_slice_buf = slice_mem_desc.phy_addr;
	}

	/* allocate frame buffers */
	ret = decoder_allocate_framebuffer(dec);
	if (ret)
		goto err4;

	if (dec->cmdl->format == STD_MJPG)
		rotid = 0;

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

err4:
	v4l_stop_capturing();
err3:
	decoder_close(dec);

	/* free the frame buffers */
	decoder_free_framebuffer(dec);
err2:
	/* close the encoder */
	encoder_close(enc);

	/* free the allocated framebuffers */
	encoder_free_framebuffer(enc);
err1:
	if (cmdl->format == STD_AVC) {
		IOFreePhyMem(&ps_mem_desc);
		IOFreePhyMem(&slice_mem_desc);
	}

	IOFreeVirtMem(&dec_mem_desc);
	IOFreePhyMem(&dec_mem_desc);
	if (dec)
		free(dec);
err:
	if (cpu_is_mx6x() && cmdl->format == STD_MPEG4 && enc->mp4_dataPartitionEnable) {
		IOFreeVirtMem(&enc_scratch_mem_desc);
		IOFreePhyMem(&enc_scratch_mem_desc);
	}
	/* free the physical memory */
	IOFreeVirtMem(&enc_mem_desc);
	IOFreePhyMem(&enc_mem_desc);
	if (enc)
		free(enc);
#ifndef COMMON_INIT
	vpu_UnInit();
#endif
	return 0;
}

