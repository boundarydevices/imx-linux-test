/*
 * Copyright (C) 2012-2013 Freescale Semiconductor, Inc.
 *
 * Copyright (c) 2006, Chips & Media.  All rights reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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

extern int quitflag;
extern int dec_fill_bsbuffer(DecHandle handle, struct cmd_line *cmd,
		u32 bs_va_startaddr, u32 bs_va_endaddr,
		u32 bs_pa_startaddr, int defaultsize,
		int *eos, int *fill_end_bs);

static int isInterlacedMPEG4 = 0;

#define JPG_HEADER_SIZE	     0x200

struct encode *enc;
struct decode *dec;
static int frameRateInfo = 0;

/*
 * This function is to convert framebuffer from interleaved Cb/Cr mode
 * to non-interleaved Cb/Cr mode.
 *
 * Note: This function does _NOT_ really store this framebuffer into file.
 */
static void
saveNV12ImageHelper(u8 *pYuv, struct decode *dec, u8 *buf)
{
	int Y, Cb;
	u8 *Y1, *Cb1, *Cr1;
	int img_size;
	int y, x;
	u8 *tmp;
	int height = dec->picheight;
	int stride = dec->stride;

	if (!pYuv || !buf) {
		err_msg("pYuv or buf should not be NULL.\n");
		return;
	}

	img_size = stride * height;

	Y = (int)buf;
	Cb = Y + img_size;

	Y1 = pYuv;
	Cb1 = Y1 + img_size;
	Cr1 = Cb1 + (img_size >> 2);

	memcpy(Y1, (u8 *)Y, img_size);

	for (y = 0; y < (dec->picheight / 2); y++) {
		tmp = (u8*)(Cb + dec->picwidth * y);
		for (x = 0; x < dec->picwidth; x += 2) {
			*Cb1++ = tmp[x];
			*Cr1++ = tmp[x + 1];
		}
	}
}

/*
 * This function is to store the framebuffer with Cropping size.
 *
 * Note: The output of picture width or height from VPU is always
 * 4-bit aligned. For example, the Cropping information in one
 * bit stream is crop.left/crop.top/crop.right/crop.bottom = 0/0/320/136
 * VPU output is picWidth = 320, picHeight = (136 + 15) & ~15 = 144;
 * whereas, this function will use crop information to save the framebuffer
 * with picWidth = 320, picHeight = 136. Kindly remind that all this calculation
 * is under the case without Rotation or Mirror.
 */
static void
saveCropYuvImageHelper(struct decode *dec, u8 *buf, Rect cropRect)
{
	u8 *pCropYuv;
	int cropWidth, cropHeight;
	int i;

	int width = dec->picwidth;
	int height = dec->picheight;
	int rot_en = dec->cmdl->rot_en;
	int rot_angle = dec->cmdl->rot_angle;

	if (!buf) {
		err_msg("buffer point should not be NULL.\n");
		return;
	}

	if (rot_en && (rot_angle == 90 || rot_angle == 270)) {
		i = width;
		width = height;
		height = i;
	}

	dprintf(3, "left/top/right/bottom = %lu/%lu/%lu/%lu\n", cropRect.left,
			cropRect.top, cropRect.right, cropRect.bottom);

	pCropYuv = buf;
	cropWidth = cropRect.right - cropRect.left;
	cropHeight = cropRect.bottom - cropRect.top;

	pCropYuv += width * cropRect.top;
	pCropYuv += cropRect.left;

	for (i = 0; i < cropHeight; i++) {
		fwriten(dec->cmdl->dst_fd, pCropYuv, cropWidth);
		pCropYuv += width;
	}

	if (dec->cmdl->format == STD_MJPG && dec->mjpg_fmt == MODE400)
		return;

	cropWidth /= 2;
	cropHeight /= 2;
	pCropYuv = buf + (width * height);
	pCropYuv += (width / 2) * (cropRect.top / 2);
	pCropYuv += cropRect.left / 2;

	for (i = 0; i < cropHeight; i++) {
		fwriten(dec->cmdl->dst_fd, pCropYuv, cropWidth);
		pCropYuv += width / 2;
	}

	pCropYuv = buf + (width * height) * 5 / 4;
	pCropYuv += (width / 2) * (cropRect.top / 2);
	pCropYuv += cropRect.left / 2;

	for (i = 0; i < cropHeight; i++) {
		fwriten(dec->cmdl->dst_fd, pCropYuv, cropWidth);
		pCropYuv += width / 2;
	}
}

/*
 * This function is to swap the cropping left/top/right/bottom
 * when there's cropping information, under rotation case.
 *
 * Note: If there's no cropping information in bit stream, just
 *	set rotCrop as no cropping info. And hence, the calling
 *	function after this will handle this case.
 */
static void
swapCropRect(struct decode *dec, Rect *rotCrop)
{
	int mode = 0;
	int rot_en = dec->cmdl->rot_en;
	int rotAngle = dec->cmdl->rot_angle;
	int framebufWidth = dec->picwidth;
	int framebufHeight = dec->picheight;
	int mirDir = dec->cmdl->mirror;
	int crop;

	if (!rot_en)
		err_msg("VPU Rotation disabled. No need to call this func.\n");

	Rect *norCrop = &(dec->picCropRect);
	dprintf(3, "left/top/right/bottom = %lu/%lu/%lu/%lu\n", norCrop->left,
			norCrop->top, norCrop->right, norCrop->bottom);

	/*
	 * If no cropping info in bitstream, we just set rotCrop as it is.
	 */
	crop = norCrop->left | norCrop->top | norCrop->right | norCrop->bottom;
	if (!crop) {
		memcpy(rotCrop, norCrop, sizeof(*norCrop));
		return;
	}

	switch (rotAngle) {
		case 0:
			switch (mirDir) {
				case MIRDIR_NONE:
					mode = 0;
					break;
				case MIRDIR_VER:
					mode = 5;
					break;
				case MIRDIR_HOR:
					mode = 4;
					break;
				case MIRDIR_HOR_VER:
					mode = 2;
					break;
			}
			break;
		/*
		 * Remember? VPU sets the rotation angle by counter-clockwise.
		 * We convert it to clockwise, in order to make it consistent
		 * with V4L2 rotation angle strategy. (refer to decoder_start)
		 *
		 * Note: if you wanna leave VPU rotation angle as it originally
		 *	is, bear in mind you need exchange the codes for
		 *	"case 90" and "case 270".
		 */
		case 90:
			switch (mirDir) {
				case MIRDIR_NONE:
					mode = 3;
					break;
				case MIRDIR_VER:
					mode = 7;
					break;
				case MIRDIR_HOR:
					mode = 6;
					break;
				case MIRDIR_HOR_VER:
					mode = 1;
					break;
			}
			break;
		case 180:
			switch (mirDir) {
				case MIRDIR_NONE:
					mode = 2;
					break;
				case MIRDIR_VER:
					mode = 4;
					break;
				case MIRDIR_HOR:
					mode = 5;
					break;
				case MIRDIR_HOR_VER:
					mode = 0;
					break;
			}
			break;
		/*
		 * Ditto. As the rot angle 90.
		 */
		case 270:
			switch (mirDir) {
				case MIRDIR_NONE:
					mode = 1;
					break;
				case MIRDIR_VER:
					mode = 6;
					break;
				case MIRDIR_HOR:
					mode = 7;
					break;
				case MIRDIR_HOR_VER:
					mode = 3;
					break;
			}
			break;
	}

	switch (mode) {
		case 0:
			rotCrop->bottom = norCrop->bottom;
			rotCrop->left = norCrop->left;
			rotCrop->right = norCrop->right;
			rotCrop->top = norCrop->top;
			break;
		case 1:
			rotCrop->bottom = framebufWidth - norCrop->left;
			rotCrop->left = norCrop->top;
			rotCrop->right = norCrop->bottom;
			rotCrop->top = framebufWidth - norCrop->right;
			break;
		case 2:
			rotCrop->bottom = framebufHeight - norCrop->top;
			rotCrop->left = framebufWidth - norCrop->right;
			rotCrop->right = framebufWidth - norCrop->left;
			rotCrop->top = framebufHeight - norCrop->bottom;
			break;
		case 3:
			rotCrop->bottom = norCrop->right;
			rotCrop->left = framebufHeight - norCrop->bottom;
			rotCrop->right = framebufHeight - norCrop->top;
			rotCrop->top = norCrop->left;
			break;
		case 4:
			rotCrop->bottom = norCrop->bottom;
			rotCrop->left = framebufWidth - norCrop->right;
			rotCrop->right = framebufWidth - norCrop->left;
			rotCrop->top = norCrop->top;
			break;
		case 5:
			rotCrop->bottom = framebufHeight - norCrop->top;
			rotCrop->left = norCrop->left;
			rotCrop->right = norCrop->right;
			rotCrop->top = framebufHeight - norCrop->bottom;
			break;
		case 6:
			rotCrop->bottom = norCrop->right;
			rotCrop->left = norCrop->top;
			rotCrop->right = norCrop->bottom;
			rotCrop->top = norCrop->left;
			break;
		case 7:
			rotCrop->bottom = framebufWidth - norCrop->left;
			rotCrop->left = framebufHeight - norCrop->bottom;
			rotCrop->right = framebufHeight - norCrop->top;
			rotCrop->top = framebufWidth - norCrop->right;
			break;
	}

	return;
}

/*
 * This function is to store the framebuffer into file.
 * It will handle the cases of chromaInterleave, or cropping,
 * or both.
 */
static void
write_to_file(struct decode *dec, Rect cropRect, int index)
{
	int height = (dec->picheight + 15) & ~15 ;
	int stride = dec->stride;
	int chromaInterleave = dec->cmdl->chromaInterleave;
	int img_size;
	u8 *pYuv = NULL, *pYuv0 = NULL, *buf;
	struct frame_buf *pfb = NULL;
	int cropping;

	dprintf(3, "left/top/right/bottom = %lu/%lu/%lu/%lu\n", cropRect.left,
			cropRect.top, cropRect.right, cropRect.bottom);
	cropping = cropRect.left | cropRect.top | cropRect.bottom | cropRect.right;

	pfb = dec->pfbpool[index];
	buf = (u8 *)(pfb->addrY + pfb->desc.virt_uaddr - pfb->desc.phy_addr);

	img_size = stride * height * 3 / 2;
	if (dec->cmdl->format == STD_MJPG) {
		if (dec->mjpg_fmt == MODE422 || dec->mjpg_fmt == MODE224)
			img_size = stride * height * 2;
		else if (dec->mjpg_fmt == MODE400)
			img_size = stride * height;
		else if (dec->mjpg_fmt == MODE444)
			img_size = stride * height * 3;
	}

	if (chromaInterleave == 0 && cropping == 0) {
		fwriten(dec->cmdl->dst_fd, buf, img_size);
		goto out;
	}

	/*
	 * There could be these three cases:
	 * interleave == 0, cropping == 1
	 * interleave == 1, cropping == 0
	 * interleave == 1, cropping == 1
	 */
	if (chromaInterleave) {
		if (!pYuv) {
			pYuv0 = pYuv = malloc(img_size);
			if (!pYuv)
				err_msg("malloc error\n");
		}

		saveNV12ImageHelper(pYuv, dec, buf);
	}

	if (cropping) {
		if (!pYuv0) {
			pYuv0 = pYuv = malloc(img_size);
			if (!pYuv)
				err_msg("malloc error\n");

			saveCropYuvImageHelper(dec, buf, cropRect);
		} else {/* process the buf beginning at pYuv0 again for Crop. */
			pYuv = pYuv0;
			saveCropYuvImageHelper(dec, pYuv, cropRect);
		}

	} else {
		fwriten(dec->cmdl->dst_fd, (u8 *)pYuv0, img_size);
	}

out:
	if (pYuv0) {
		free(pYuv0);
		pYuv0 = NULL;
		pYuv = NULL;
	}

	return;
}


static int
enc_readbs_ring_buffer(EncHandle handle, struct cmd_line *cmd,
                u32 bs_va_startaddr, u32 bs_va_endaddr, u32 bs_pa_startaddr,
                int defaultsize)
{
	RetCode ret;
	int space = 0, room;
	PhysicalAddress pa_read_ptr, pa_write_ptr;
	u32 target_addr, size;

	ret = vpu_EncGetBitstreamBuffer(handle, &pa_read_ptr, &pa_write_ptr,
				        &size);
	if (ret != RETCODE_SUCCESS) {
		err_msg("EncGetBitstreamBuffer failed\n");
		return -1;
	}

        /* No space in ring buffer */
	if (size <= 0)
		return 0;

	if (defaultsize > 0) {
		if (size < defaultsize)
			return 0;

		space = defaultsize;
	} else {
		space = size;
	}

	if (space > 0) {
		target_addr = bs_va_startaddr + (pa_read_ptr - bs_pa_startaddr);
		if ( (target_addr + space) > bs_va_endaddr) {
			room = bs_va_endaddr - target_addr;
			vpu_write(cmd, (void *)target_addr, room);
			vpu_write(cmd, (void *)bs_va_startaddr,(space - room));
		} else {
			vpu_write(cmd, (void *)target_addr, space);
		}

		ret = vpu_EncUpdateBitstreamBuffer(handle, space);
		if (ret != RETCODE_SUCCESS) {
			err_msg("EncUpdateBitstreamBuffer failed\n");
			return -1;
		}
	}

	return space;
}

static int
enc_readbs_reset_buffer(struct encode *enc, PhysicalAddress paBsBufAddr, int bsBufsize)
{
	u32 vbuf;

	vbuf = enc->virt_bsbuf_addr + paBsBufAddr - enc->phy_bsbuf_addr;
	return vpu_write(enc->cmdl, (void *)vbuf, bsBufsize);
}


static int
encoder_fill_headers(struct encode *enc)
{
	EncHeaderParam enchdr_param = {0};
	EncHandle handle = enc->handle;
	RetCode ret;
	int mbPicNum;

	/* Must put encode header before encoding */
	if (enc->cmdl->format == STD_MPEG4) {
		enchdr_param.headerType = VOS_HEADER;

		if (cpu_is_mx6x())
			goto put_mp4header;
		/*
		 * Please set userProfileLevelEnable to 0 if you need to generate
		 * user profile and level automaticaly by resolution, here is one
		 * sample of how to work when userProfileLevelEnable is 1.
		 */
		enchdr_param.userProfileLevelEnable = 1;
		mbPicNum = ((enc->enc_picwidth + 15) / 16) *((enc->enc_picheight + 15) / 16);
		if (enc->enc_picwidth <= 176 && enc->enc_picheight <= 144 &&
		    mbPicNum * frameRateInfo <= 1485)
			enchdr_param.userProfileLevelIndication = 8; /* L1 */
		/* Please set userProfileLevelIndication to 8 if L0 is needed */
		else if (enc->enc_picwidth <= 352 && enc->enc_picheight <= 288 &&
			 mbPicNum * frameRateInfo <= 5940)
			enchdr_param.userProfileLevelIndication = 2; /* L2 */
		else if (enc->enc_picwidth <= 352 && enc->enc_picheight <= 288 &&
			 mbPicNum * frameRateInfo <= 11880)
			enchdr_param.userProfileLevelIndication = 3; /* L3 */
		else if (enc->enc_picwidth <= 640 && enc->enc_picheight <= 480 &&
			 mbPicNum * frameRateInfo <= 36000)
			enchdr_param.userProfileLevelIndication = 4; /* L4a */
		else if (enc->enc_picwidth <= 720 && enc->enc_picheight <= 576 &&
			 mbPicNum * frameRateInfo <= 40500)
			enchdr_param.userProfileLevelIndication = 5; /* L5 */
		else
			enchdr_param.userProfileLevelIndication = 6; /* L6 */

put_mp4header:
		vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &enchdr_param);
		if (enc->ringBufferEnable == 0 ) {
			ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
			if (ret < 0)
				return -1;
		}

		enchdr_param.headerType = VIS_HEADER;
		vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &enchdr_param);
		if (enc->ringBufferEnable == 0 ) {
			ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
			if (ret < 0)
				return -1;
		}
		enchdr_param.headerType = VOL_HEADER;
		vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &enchdr_param);
		if (enc->ringBufferEnable == 0 ) {
			ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
			if (ret < 0)
				return -1;
		}
	} else if (enc->cmdl->format == STD_AVC) {
		enchdr_param.headerType = SPS_RBSP;
		vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
		if (enc->ringBufferEnable == 0 ) {
			ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
			if (ret < 0)
				return -1;
		}

		enchdr_param.headerType = PPS_RBSP;
		vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
		if (enc->ringBufferEnable == 0 ) {
			ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
			if (ret < 0)
				return -1;
		}
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
				vpu_write(enc->cmdl, (void *)enchdr_param.pParaSet, enchdr_param.size);
				free(enchdr_param.pParaSet);
			} else {
				err_msg("memory allocate failure\n");
				return -1;
			}
		}
	}

	return 0;
}

int
transcode_start(struct decode *dec, struct encode *enc)
{
	DecHandle handle = dec->handle;
	DecOutputInfo outinfo = {0};
	DecParam decparam = {0};
	int rot_en = dec->cmdl->rot_en, rot_stride, fwidth, fheight;
	int rot_angle = dec->cmdl->rot_angle;
	int dering_en = dec->cmdl->dering_en;
	FrameBuffer *fb = dec->fb;
	struct vpu_display *disp = dec->disp;
	int err = 0, eos = 0, fill_end_bs = 0, decodefinish = 0;
	struct timeval tdec_begin,tdec_end, total_start, total_end;
	RetCode ret = 0;
	int sec, usec, loop_id;
	double tdec_time = 0, frame_id = 0, total_time=0;
	int decIndex = 0;
	int rotid = 0, mirror;
	int count = dec->cmdl->count;
	int totalNumofErrMbs = 0;
	int disp_clr_index = -1, actual_display_index = -1, field = V4L2_FIELD_NONE;
	int is_waited_int = 0;
	int tiled2LinearEnable = dec->tiled2LinearEnable;
	char *delay_ms, *endptr;

	double tdecenc_time = 0, tenc_time=0;
	struct timeval tdecenc_begin,tdecenc_end;
	struct timeval tenc_begin,tenc_end;
	EncHandle enc_handle = enc->handle;
	EncParam  enc_param = {0};
	EncOutputInfo enc_outinfo = {0};
	int src_fbid = enc->src_fbid, enc_img_size;
	FrameBuffer *enc_fb = enc->fb;
	u32 virt_bsbuf_start = enc->virt_bsbuf_addr;
	u32 virt_bsbuf_end = virt_bsbuf_start + STREAM_BUF_SIZE;
	PhysicalAddress phy_bsbuf_start = enc->phy_bsbuf_addr;

	/* Must put encode header before encoding */
	ret = encoder_fill_headers(enc);
	if (ret) {
		err_msg("Encode fill headers failed\n");
		return -1;
	}
	enc->cmdl->src_scheme = -1;

	if ((enc_param.encLeftOffset + enc->enc_picwidth) > enc->src_picwidth) {
		err_msg("Configure is failure for width and left offset\n");
		return -1;
	}
	if ((enc_param.encTopOffset + enc->enc_picheight) > enc->src_picheight) {
		err_msg("Configure is failure for height and top offset\n");
		return -1;
	}

	if ((dec->cmdl->dst_scheme == PATH_V4L2) && (dec->cmdl->ipu_rot_en))
		rot_en = 0;

	if (rot_en || dering_en || tiled2LinearEnable) {
		rotid = dec->regfbcount;
	}

	if (dec->cmdl->format == STD_MJPG)
		rotid = 0;

	decparam.dispReorderBuf = 0;

	if (!cpu_is_mx6x()) {
		decparam.prescanEnable = dec->cmdl->prescan;
		decparam.prescanMode = 0;
	}

	decparam.skipframeMode = 0;
	decparam.skipframeNum = 0;
	/*
	 * once iframeSearchEnable is enabled, prescanEnable, prescanMode
	 * and skipframeMode options are ignored.
	 */
	decparam.iframeSearchEnable = 0;

	fwidth = ((dec->picwidth + 15) & ~15);
	fheight = ((dec->picheight + 15) & ~15);

	if (rot_en || dering_en || tiled2LinearEnable || (dec->cmdl->format == STD_MJPG)) {
		/*
		 * VPU is setting the rotation angle by counter-clockwise.
		 * We convert it to clockwise, which is consistent with V4L2
		 * rotation angle strategy.
		 */
		if (rot_en) {
			if (rot_angle == 90 || rot_angle == 270)
				rot_angle = 360 - rot_angle;
		} else
			rot_angle = 0;

		vpu_DecGiveCommand(handle, SET_ROTATION_ANGLE,
					&rot_angle);

		mirror = dec->cmdl->mirror;
		vpu_DecGiveCommand(handle, SET_MIRROR_DIRECTION,
					&mirror);

		if (rot_en)
			rot_stride = (rot_angle == 90 || rot_angle == 270) ?
					fheight : fwidth;
		else
			rot_stride = fwidth;
		vpu_DecGiveCommand(handle, SET_ROTATOR_STRIDE, &rot_stride);
	}

	gettimeofday(&total_start, NULL);

	while (1) {

		if (rot_en || dering_en || tiled2LinearEnable || (dec->cmdl->format == STD_MJPG)) {
			vpu_DecGiveCommand(handle, SET_ROTATOR_OUTPUT,
						(void *)&fb[rotid]);
			if (frame_id == 0) {
				if (rot_en) {
					vpu_DecGiveCommand(handle,
							ENABLE_ROTATION, 0);
					vpu_DecGiveCommand(handle,
							ENABLE_MIRRORING,0);
				}
				if (dering_en) {
					vpu_DecGiveCommand(handle,
							ENABLE_DERING, 0);
				}
			}
		}

		/*
		 * For mx6x MJPG decoding with streaming mode
		 * bitstream buffer filling cannot be done when JPU is in decoding,
		 * there are three places can do this:
		 * 1. before vpu_DecStartOneFrame;
		 * 2. in the case of RETCODE_JPEG_BIT_EMPTY returned in DecStartOneFrame() func;
		 * 3. after vpu_DecGetOutputInfo.
		 */
		if (cpu_is_mx6x() && (dec->cmdl->format == STD_MJPG)) {
			err = dec_fill_bsbuffer(handle, dec->cmdl,
				    dec->virt_bsbuf_addr,
				    (dec->virt_bsbuf_addr + STREAM_BUF_SIZE),
				    dec->phy_bsbuf_addr, STREAM_FILL_SIZE,
				    &eos, &fill_end_bs);
			if (err < 0) {
				err_msg("dec_fill_bsbuffer failed\n");
				return -1;
			}
		}

                gettimeofday(&tdecenc_begin, NULL);
		gettimeofday(&tdec_begin, NULL);
		ret = vpu_DecStartOneFrame(handle, &decparam);
		if (ret == RETCODE_JPEG_EOS) {
			info_msg(" JPEG bitstream is end\n");
			break;
		} else if (ret == RETCODE_JPEG_BIT_EMPTY) {
			err = dec_fill_bsbuffer(handle, dec->cmdl,
				    dec->virt_bsbuf_addr,
				    (dec->virt_bsbuf_addr + STREAM_BUF_SIZE),
				    dec->phy_bsbuf_addr, STREAM_FILL_SIZE,
				    &eos, &fill_end_bs);
			if (err < 0) {
				err_msg("dec_fill_bsbuffer failed\n");
				return -1;
			}
			continue;
		}

		if (ret != RETCODE_SUCCESS) {
			err_msg("DecStartOneFrame failed, ret=%d\n", ret);
			return -1;
		}

		is_waited_int = 0;
		loop_id = 0;
		while (vpu_IsBusy()) {
			if (!(cpu_is_mx6x() && (dec->cmdl->format == STD_MJPG))) {
				err = dec_fill_bsbuffer(handle, dec->cmdl,
					dec->virt_bsbuf_addr,
					(dec->virt_bsbuf_addr + STREAM_BUF_SIZE),
					dec->phy_bsbuf_addr, STREAM_FILL_SIZE,
					&eos, &fill_end_bs);
				if (err < 0) {
					err_msg("dec_fill_bsbuffer failed\n");
					return -1;
				}
			}
			/*
			 * Suppose vpu is hang if one frame cannot be decoded in 5s,
			 * then do vpu software reset.
			 * Please take care of this for network case since vpu
			 * interrupt also cannot be received if no enough data.
			 */
			if (loop_id == 50) {
				err = vpu_SWReset(handle, 0);
				return -1;
			}

			vpu_WaitForInt(100);
			is_waited_int = 1;
			loop_id ++;
		}

		if (!is_waited_int)
			vpu_WaitForInt(100);

		gettimeofday(&tdec_end, NULL);
		sec = tdec_end.tv_sec - tdec_begin.tv_sec;
		usec = tdec_end.tv_usec - tdec_begin.tv_usec;

		if (usec < 0) {
			sec--;
			usec = usec + 1000000;
		}

		tdec_time += (sec * 1000000) + usec;

		ret = vpu_DecGetOutputInfo(handle, &outinfo);

		enc_img_size = enc->src_picwidth * enc->src_picheight;
		if (outinfo.indexFrameDisplay >= 0) {
			enc_fb[src_fbid].myIndex = enc->src_fbid;
			enc_fb[src_fbid].bufY = disp->buffers[outinfo.indexFrameDisplay]->offset;
			enc_fb[src_fbid].bufCb = enc_fb[src_fbid].bufY + enc_img_size;
			enc_fb[src_fbid].bufCr = enc_fb[src_fbid].bufCb + (enc_img_size >> 2);
			enc_fb[src_fbid].strideY = enc->src_picwidth;
			enc_fb[src_fbid].strideC = enc->src_picwidth / 2;

			enc_param.sourceFrame = &enc_fb[src_fbid];
			enc_param.quantParam = enc->cmdl->quantParam;
			if (enc_param.quantParam <= 0)
				enc_param.quantParam = 20;
			enc_param.forceIPicture = 0;
			enc_param.skipPicture = 0;
			enc_param.enableAutoSkip = 0;
			enc_param.encLeftOffset = 0;
			enc_param.encTopOffset = 0;
		}

		gettimeofday(&tenc_begin, NULL);
		if (outinfo.indexFrameDisplay >= 0) {
			ret = vpu_EncStartOneFrame(enc_handle, &enc_param);
                }
		if (ret != RETCODE_SUCCESS) {
			err_msg("EncStartOneFrame failed\n");
			return -1;
		}

		if ((dec->cmdl->format == STD_MJPG) &&
		    (outinfo.indexFrameDisplay == 0)) {
			outinfo.indexFrameDisplay = rotid;
		}

		dprintf(4, "frame_id = %d\n", (int)frame_id);
		if (ret != RETCODE_SUCCESS) {
			err_msg("vpu_DecGetOutputInfo failed Err code is %d\n"
				"\tframe_id = %d\n", ret, (int)frame_id);
			return -1;
		}

		if (outinfo.decodingSuccess == 0) {
			warn_msg("Incomplete finish of decoding process.\n"
				"\tframe_id = %d\n", (int)frame_id);
			if (quitflag)
				break;
			else
				continue;
		}

		if (cpu_is_mx6x() && (outinfo.decodingSuccess & 0x10)) {
			warn_msg("vpu needs more bitstream in rollback mode\n"
				"\tframe_id = %d\n", (int)frame_id);

			err = dec_fill_bsbuffer(handle,  dec->cmdl, dec->virt_bsbuf_addr,
					(dec->virt_bsbuf_addr + STREAM_BUF_SIZE),
					dec->phy_bsbuf_addr, 0, &eos, &fill_end_bs);
			if (err < 0) {
				err_msg("dec_fill_bsbuffer failed\n");
				return -1;
			}

			if (quitflag)
				break;
			else
				continue;
		}

		if (outinfo.notSufficientPsBuffer) {
			err_msg("PS Buffer overflow\n");
			return -1;
		}

		if (outinfo.notSufficientSliceBuffer) {
			err_msg("Slice Buffer overflow\n");
			return -1;
		}

		if (outinfo.indexFrameDecoded >= 0) {
			if (dec->cmdl->format == STD_VC1) {
				if (outinfo.pictureStructure == 2)
					info_msg("dec_idx %d : FRAME_INTERLACE\n", decIndex);
				else if (outinfo.pictureStructure == 3) {
					if (outinfo.topFieldFirst)
						field = V4L2_FIELD_INTERLACED_TB;
					else
						field = V4L2_FIELD_INTERLACED_BT;
				}
				if (outinfo.vc1_repeatFrame)
					info_msg("dec_idx %d : VC1 RPTFRM [%1d]\n", decIndex, outinfo.vc1_repeatFrame);
			} else if ((dec->cmdl->format == STD_AVC) ||
				   (dec->cmdl->format == STD_MPEG4)) {
				if ((outinfo.interlacedFrame) ||
				    ((dec->cmdl->format == STD_MPEG4) && isInterlacedMPEG4)) {
					if (outinfo.topFieldFirst)
						field = V4L2_FIELD_INTERLACED_TB;
					else
						field = V4L2_FIELD_INTERLACED_BT;
					dprintf(3, "Top Field First flag: %d, dec_idx %d\n",
						  outinfo.topFieldFirst, decIndex);
				}
			} else if ((dec->cmdl->format != STD_MPEG4) &&
				   (dec->cmdl->format != STD_H263) &&
				   (dec->cmdl->format != STD_RV)){
				if (outinfo.interlacedFrame) {
					if (outinfo.pictureStructure == 1)
						field = V4L2_FIELD_TOP;
					else if (outinfo.pictureStructure == 2)
						field = V4L2_FIELD_BOTTOM;
					else if (outinfo.pictureStructure == 3) {
						if (outinfo.topFieldFirst)
							field = V4L2_FIELD_INTERLACED_TB;
						else
							field = V4L2_FIELD_INTERLACED_BT;
					}
				}
				if (outinfo.repeatFirstField)
					info_msg("frame_idx %d : Repeat First Field\n", decIndex);
			}
		}

		if(outinfo.indexFrameDecoded >= 0)
			decIndex++;

		if (outinfo.indexFrameDisplay == -1)
			decodefinish = 1;
		else if ((outinfo.indexFrameDisplay > dec->regfbcount) &&
			 (outinfo.prescanresult != 0) && !cpu_is_mx6x())
			decodefinish = 1;

		if (decodefinish)
			break;

		if (!cpu_is_mx6x() && (outinfo.prescanresult == 0) &&
		    (decparam.prescanEnable == 1)) {
			if (eos) {
				break;
			} else {
				int fillsize = 0;

				if (dec->cmdl->src_scheme == PATH_NET)
					fillsize = 1000;
				else
					warn_msg("Prescan: not enough bs data"
									"\n");

				dec->cmdl->complete = 1;
				err = dec_fill_bsbuffer(handle,
						dec->cmdl,
					        dec->virt_bsbuf_addr,
					        (dec->virt_bsbuf_addr +
						 STREAM_BUF_SIZE),
					        dec->phy_bsbuf_addr, fillsize,
					        &eos, &fill_end_bs);
				dec->cmdl->complete = 0;
				if (err < 0) {
					err_msg("dec_fill_bsbuffer failed\n");
					return -1;
				}

				if (eos)
					break;
				else
					continue;
			}
		}

		if (quitflag)
			break;

		/* BIT don't have picture to be displayed */
		if ((outinfo.indexFrameDisplay == -3) ||
				(outinfo.indexFrameDisplay == -2)) {
			dprintf(3, "VPU doesn't have picture to be displayed.\n"
				"\toutinfo.indexFrameDisplay = %d\n",
						outinfo.indexFrameDisplay);

			if (dec->cmdl->format != STD_MJPG && disp_clr_index >= 0) {
				err = vpu_DecClrDispFlag(handle, disp_clr_index);
				if (err)
					err_msg("vpu_DecClrDispFlag failed Error code"
							" %d\n", err);
			}
			disp_clr_index = outinfo.indexFrameDisplay;
			continue;
		}

		if (rot_en || dering_en || tiled2LinearEnable || (dec->cmdl->format == STD_MJPG))
			actual_display_index = rotid;
		else
			actual_display_index = outinfo.indexFrameDisplay;

		if (dec->cmdl->dst_scheme == PATH_V4L2) {
			err = v4l_put_data(dec, actual_display_index, field, dec->cmdl->fps);

			if (err)
				return -1;

			if (dec->cmdl->dst_scheme == PATH_V4L2) {
				if (dec->cmdl->format != STD_MJPG && disp_clr_index >= 0) {
					err = vpu_DecClrDispFlag(handle, disp_clr_index);
					if (err)
						err_msg("vpu_DecClrDispFlag failed Error code"
							" %d\n", err);
				}

				if (dec->cmdl->format == STD_MJPG) {
					rotid++;
					rotid %= dec->regfbcount;
				} else if (rot_en || dering_en || tiled2LinearEnable) {
					disp_clr_index = outinfo.indexFrameDisplay;
					if (disp->buf.index != -1)
						rotid = disp->buf.index; /* de-queued buffer as next rotation buffer */
					else
						rotid++;
				}
				else
					disp_clr_index = disp->buf.index;
			}
		} else {
			if (rot_en) {
				Rect rotCrop;
				swapCropRect(dec, &rotCrop);
				write_to_file(dec, rotCrop, actual_display_index);
			} else {
				write_to_file(dec, dec->picCropRect, actual_display_index);
			}

			if (dec->cmdl->format != STD_MJPG && disp_clr_index >= 0) {
				err = vpu_DecClrDispFlag(handle,disp_clr_index);
				if (err)
					err_msg("vpu_DecClrDispFlag failed Error code"
						" %d\n", err);
			}
			disp_clr_index = outinfo.indexFrameDisplay;
		}

		if (outinfo.numOfErrMBs) {
			if (cpu_is_mx6x() && dec->cmdl->format == STD_MJPG)
				info_msg("Error Mb info:0x%x, in Frame : %d\n",
					    outinfo.numOfErrMBs, (int)frame_id);
			else {
				totalNumofErrMbs += outinfo.numOfErrMBs;
				info_msg("Num of Error Mbs : %d, in Frame : %d \n",
					    outinfo.numOfErrMBs, (int)frame_id);
			}
		}

		if ((count != 0) && (frame_id >= count))
			break;

		if (dec->cmdl->src_scheme == PATH_NET) {
			err = dec_fill_bsbuffer(handle,	dec->cmdl,
				      dec->virt_bsbuf_addr,
				      (dec->virt_bsbuf_addr + STREAM_BUF_SIZE),
				      dec->phy_bsbuf_addr, STREAM_FILL_SIZE,
				      &eos, &fill_end_bs);
			if (err < 0) {
				err_msg("dec_fill_bsbuffer failed\n");
				return -1;
			}
		}

	        delay_ms = getenv("VPU_DECODER_DELAY_MS");
	        if (delay_ms && strtol(delay_ms, &endptr, 10))
		        usleep(strtol(delay_ms,&endptr, 10) * 1000);

		loop_id = 0;
		while (vpu_IsBusy()) {
			vpu_WaitForInt(100);
			if (loop_id == 10) {
				ret = vpu_SWReset(enc_handle, 0);
				warn_msg("vpu_SWReset in dec\n");
				return -1;
			}
			loop_id ++;
		}

		if (outinfo.indexFrameDisplay >= 0)
			ret = vpu_EncGetOutputInfo(enc_handle, &enc_outinfo);

		if (ret != RETCODE_SUCCESS) {
			err_msg("EncGetOutputInfo failed\n");
			return -1;
		}

		if (outinfo.indexFrameDisplay >= 0) {
			if (enc->ringBufferEnable == 0) {
				ret = enc_readbs_reset_buffer(enc, enc_outinfo.bitstreamBuffer,
								    enc_outinfo.bitstreamSize);
				if (ret < 0) {
					err_msg("writing bitstream buffer failed\n");
					goto err2;
				}
			} else
				enc_readbs_ring_buffer(enc_handle, enc->cmdl, virt_bsbuf_start,
							    virt_bsbuf_end, phy_bsbuf_start, 0);
		}

		gettimeofday(&tenc_end, NULL);
		sec = tenc_end.tv_sec - tenc_begin.tv_sec;
		usec = tenc_end.tv_usec - tenc_begin.tv_usec;
		if (usec < 0) {
			sec--;
			usec = usec + 1000000;
		}
		tenc_time += (sec * 1000000) + usec;

		gettimeofday(&tdecenc_end, NULL);
		sec = tdecenc_end.tv_sec - tdecenc_begin.tv_sec;
		usec = tdecenc_end.tv_usec - tdecenc_begin.tv_usec;
		if (usec < 0) {
			sec--;
			usec = usec + 1000000;
		}
		tdecenc_time += (sec * 1000000) + usec;
		frame_id++;
	}  /* end of while loop */


	if (totalNumofErrMbs) {
		info_msg("Total Num of Error MBs : %d\n", totalNumofErrMbs);
	}

	gettimeofday(&total_end, NULL);
	sec = total_end.tv_sec - total_start.tv_sec;
	usec = total_end.tv_usec - total_start.tv_usec;
	if (usec < 0) {
		sec--;
		usec = usec + 1000000;
	}
	total_time = (sec * 1000000) + usec;

	info_msg("%d frames took %d microseconds\n", (int)frame_id,
						(int)total_time);
	info_msg("average fps for dec alone (no disp):  fps = %.2f \n", (frame_id / (tdec_time / 1000000)));
	info_msg("average fps for enc (with disp):      fps = %.2f \n", (frame_id / (tenc_time / 1000000)));
	info_msg("average fps for transcode with disp:  fps = %.2f\n",  (frame_id / (tdecenc_time / 1000000)));
	info_msg("average fps for total with disp:      fps = %.2f \n", (frame_id / (total_time / 1000000)));

err2:
	/* Inform the other end that no more frames will be sent */
	if (enc->cmdl->dst_scheme == PATH_NET) {
		vpu_write(enc->cmdl, NULL, 0);
	}

	return 0;
}

int
transcode_test(void *arg)
{
	struct cmd_line *cmdl = (struct cmd_line *)arg;
	vpu_mem_desc mem_desc = {0};
	vpu_mem_desc ps_mem_desc = {0};
	vpu_mem_desc slice_mem_desc = {0};
	vpu_mem_desc vp8_mbparam_mem_desc = {0};
	struct decode *dec;
	int ret, eos = 0, fill_end_bs = 0, fillsize = 0;

        vpu_mem_desc enc_mem_desc = {0};
        vpu_mem_desc enc_scratch_mem_desc = {0};
        struct encode *enc;

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

	dec = (struct decode *)calloc(1, sizeof(struct decode));
	if (dec == NULL) {
		err_msg("Failed to allocate decode structure\n");
		return -1;
	}

	mem_desc.size = STREAM_BUF_SIZE;
	ret = IOGetPhyMem(&mem_desc);
	if (ret) {
		err_msg("Unable to obtain physical mem\n");
		return -1;
	}

	if (IOGetVirtMem(&mem_desc) <= 0) {
		err_msg("Unable to obtain virtual mem\n");
		IOFreePhyMem(&mem_desc);
		free(dec);
		return -1;
	}

	dec->phy_bsbuf_addr = mem_desc.phy_addr;
	dec->virt_bsbuf_addr = mem_desc.virt_uaddr;

	dec->reorderEnable = 1;
	dec->tiled2LinearEnable = 0;

	dec->cmdl = cmdl;

        /* Always set to v4l2 path for display so the "-o" option
         * becomes the output for writing the encoded bitstream */
	dec->cmdl->dst_scheme = PATH_V4L2;

	if (cmdl->format == STD_AVC) {
		ps_mem_desc.size = PS_SAVE_SIZE;
		ret = IOGetPhyMem(&ps_mem_desc);
		if (ret) {
			err_msg("Unable to obtain physical ps save mem\n");
			goto err;
		}
		dec->phy_ps_buf = ps_mem_desc.phy_addr;

	}

	/* open decoder */
	ret = decoder_open(dec);
	if (ret)
		goto err;

	cmdl->complete = 1;
	if (dec->cmdl->src_scheme == PATH_NET)
		fillsize = 1024;

	ret = dec_fill_bsbuffer(dec->handle, cmdl,
			dec->virt_bsbuf_addr,
		        (dec->virt_bsbuf_addr + STREAM_BUF_SIZE),
			dec->phy_bsbuf_addr, fillsize, &eos, &fill_end_bs);

	cmdl->complete = 0;
	if (ret < 0) {
		err_msg("dec_fill_bsbuffer failed\n");
		goto err1;
	}

	/* parse the bitstream */
	ret = decoder_parse(dec);
	if (ret) {
		err_msg("decoder parse failed\n");
		goto err1;
	}

	/* allocate slice buf */
	if (cmdl->format == STD_AVC) {
		slice_mem_desc.size = dec->phy_slicebuf_size;
		ret = IOGetPhyMem(&slice_mem_desc);
		if (ret) {
			err_msg("Unable to obtain physical slice save mem\n");
			goto err1;
		}
		dec->phy_slice_buf = slice_mem_desc.phy_addr;
	}

	if (cmdl->format == STD_VP8) {
		vp8_mbparam_mem_desc.size = 68 * (dec->stride * dec->picwidth / 256);
		ret = IOGetPhyMem(&vp8_mbparam_mem_desc);
		if (ret) {
			err_msg("Unable to obtain physical vp8 mbparam mem\n");
			goto err1;
		}
		dec->phy_vp8_mbparam_buf = vp8_mbparam_mem_desc.phy_addr;
	}

	/* allocate frame buffers */
	ret = decoder_allocate_framebuffer(dec);
	if (ret)
		goto err1;

	/* Not set fps when doing performance test default */
        if (dec->cmdl->fps == 0)
                dec->cmdl->fps = 30;

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
	enc->cmdl->format = STD_AVC;

	ret = encoder_open(enc);
	if (ret)
		goto err3;

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
			goto err1;
		}
		enc->scratchBuf.bufferBase = enc_scratch_mem_desc.phy_addr;
		enc->scratchBuf.bufferSize = enc_scratch_mem_desc.size;
	}
	/*allocate memory for the frame buffers */
	ret = encoder_allocate_framebuffer(enc);
	if (ret)
		goto err2;

	/* start transcoding */
	ret = transcode_start(dec, enc);

err3:
	/* free the allocated framebuffers */
	encoder_free_framebuffer(enc);
err2:
	/* close the encoder */
	encoder_close(enc);

err1:
	decoder_close(dec);
	/* free the frame buffers */
	decoder_free_framebuffer(dec);
err:
	if (cmdl->format == STD_AVC) {
		IOFreePhyMem(&slice_mem_desc);
		IOFreePhyMem(&ps_mem_desc);
	}

	if (cmdl->format == STD_VP8)
		IOFreePhyMem(&vp8_mbparam_mem_desc);

	IOFreeVirtMem(&mem_desc);
	IOFreePhyMem(&mem_desc);
	free(dec);
#ifndef COMMON_INIT
	vpu_UnInit();
#endif
	return ret;
}
