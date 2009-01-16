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

extern int quitflag;

static FILE *fpFrmStatusLogfile = 0;
static FILE *fpMbLogfile = 0;
static FILE *fpMvLogfile = 0;
/*static FILE *fpUserDataLogfile = 0;*/

#define FN_FRAME_BUFFER_STATUS "log_FrameBufStatus.log"
#define FN_MB_DATA "log_MB_Para.log"
#define FN_MV_DATA "log_Mv_Para.log"
#define FN_USER_DATA "log_User_Data.log"

void SaveFrameBufStat(u8 *frmStatusBuf, int size) {

	int i;

	if (fpFrmStatusLogfile == 0) {
		fpFrmStatusLogfile = fopen(FN_FRAME_BUFFER_STATUS, "w+");
	}

	for (i = 0; i < size; i++) {
		fprintf(fpFrmStatusLogfile, "%X %X ",
				((frmStatusBuf[i] >> 4) & 0x0F),
						(frmStatusBuf[i] & 0x0F));
	}
	fprintf(fpFrmStatusLogfile, "\n");
	fflush(fpFrmStatusLogfile);
}


void SaveMB_Para(u8 *mbParaBuf, int size, int mbNumX, int DecNum) {

	int i;

	if (fpMbLogfile == 0) {
		fpMbLogfile = fopen(FN_MB_DATA, "w+");
	}

	fprintf(fpMbLogfile, "Dec Frame Num : %d\n", DecNum);

	for (i = 0; i < size; i++) {
		fprintf(fpMbLogfile, "%02X ", mbParaBuf[i]);
		if(mbNumX == ((i % mbNumX) + 1))
			fprintf(fpMbLogfile, "%%\n");
	}

	fflush(fpMbLogfile);
}

void SaveMvPara(u8 *mvParaBuf, int size, int mvNumPerMb, int mbNumX, int DecNum) {

	int i, j;
	short mvX, mvY;
	u8 *mvDatabuf;

	if (fpMvLogfile == 0) {
		fpMvLogfile = fopen(FN_MV_DATA, "w+");
	}

	fprintf(fpMvLogfile, "Dec Frame Num : %d\n", DecNum);

	for (i = 0; i < size; i++) {
		for (j = 0; j < mvNumPerMb; j++) {
			mvDatabuf = (mvParaBuf + (i*mvNumPerMb + j)*4);
			mvX = (short)((mvDatabuf[0]<<8) | (mvDatabuf[1]<<0));
			mvY = (short)((mvDatabuf[2]<<8) | (mvDatabuf[3]<<0));

			if (!(mvX & 0x8000)){
				/* Intra MB */
				mvX = 0;
				mvY = 0;

				if (j == 0 )
					fprintf(fpMvLogfile, "[ No FW  ]");
				if (j == 1)
					fprintf(fpMvLogfile, "[ No BW  ]");

			} else {

				if(mvX & 0x2000) {/* Signed extension */
					mvX = mvX | 0xC000;
				} else {
					mvX = mvX & 0x1FFF;
				}

				if(mvY & 0x2000) {
					mvY = mvY | 0xC000;
				} else {
					mvY = mvY & 0x1FFF;
				}

				if (j == 0 )
					fprintf(fpMvLogfile, "[%4d%4d]", mvX, mvY);
				if (j == 1)
					fprintf(fpMvLogfile, "[%4d%4d]", mvX, mvY);
			}
		}
		if(mvNumPerMb && mbNumX == ((i%mbNumX) + 1))
			fprintf(fpMvLogfile, "%%\n");
	}
	fflush(fpMvLogfile);
}

/*
 * Fill the bitstream ring buffer
 */
int dec_fill_bsbuffer(DecHandle handle, struct cmd_line *cmd,
		u32 bs_va_startaddr, u32 bs_va_endaddr,
		u32 bs_pa_startaddr, int defaultsize,
		int *eos, int *fill_end_bs)
{
	RetCode ret;
	PhysicalAddress pa_read_ptr, pa_write_ptr;
	u32 target_addr, space;
	int size;
	int nread, room;
	*eos = 0;

	ret = vpu_DecGetBitstreamBuffer(handle, &pa_read_ptr, &pa_write_ptr,
					&space);
	if (ret != RETCODE_SUCCESS) {
		err_msg("vpu_DecGetBitstreamBuffer failed\n");
		return -1;
	}

	/* Decoder bitstream buffer is empty */
	if (space <= 0)
		return 0;

	if (defaultsize > 0) {
		if (space < defaultsize)
			return 0;

		size = defaultsize;
	} else {
		size = ((space >> 9) << 9);
	}

	if (size == 0)
		return 0;

	/* Fill the bitstream buffer */
	target_addr = bs_va_startaddr + (pa_write_ptr - bs_pa_startaddr);
	if ( (target_addr + size) > bs_va_endaddr) {
		room = bs_va_endaddr - target_addr;
		nread = vpu_read(cmd, (void *)target_addr, room);
		if (nread <= 0) {
			/* EOF or error */
			if (nread < 0) {
				if (nread == -EAGAIN)
					return 0;
				
				return -1;
			}
			
			*eos = 1;
		} else {
			/* unable to fill the requested size, so back off! */
			if (nread != room)
				goto update;

			/* read the remaining */
			space = nread;
			nread = vpu_read(cmd, (void *)bs_va_startaddr,
					(size - room));
			if (nread <= 0) {
				/* EOF or error */
				if (nread < 0) {
					if (nread == -EAGAIN)
						return 0;
					
					return -1;
				}

				*eos = 1;
			}

			nread += space;
		}
	} else {
		nread = vpu_read(cmd, (void *)target_addr, size);
		if (nread <= 0) {
			/* EOF or error */
			if (nread < 0) {
				if (nread == -EAGAIN)
					return 0;

				return -1;
			}
			
			*eos = 1;
		}
	}

update:
	if (*eos == 0) {
		ret = vpu_DecUpdateBitstreamBuffer(handle, nread);
		if (ret != RETCODE_SUCCESS) {
			err_msg("vpu_DecUpdateBitstreamBuffer failed\n");
			return -1;
		}
		*fill_end_bs = 0;
	} else {
		if (!*fill_end_bs) {
			ret = vpu_DecUpdateBitstreamBuffer(handle,
					STREAM_END_SIZE);
			if (ret != RETCODE_SUCCESS) {
				err_msg("vpu_DecUpdateBitstreamBuffer failed"
								"\n");
				return -1;
			}
			*fill_end_bs = 1;
		}

	}

	return nread;
}

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
write_to_file(struct decode *dec, u8 *buf, Rect cropRect)
{
	int height = dec->picheight;
	int stride = dec->stride;
	int chromaInterleave = dec->cmdl->chromaInterleave;
	int img_size;
	u8 *pYuv = NULL, *pYuv0 = NULL;
	int cropping;

	dprintf(3, "left/top/right/bottom = %lu/%lu/%lu/%lu\n", cropRect.left,
			cropRect.top, cropRect.right, cropRect.bottom);
	cropping = cropRect.left | cropRect.top | cropRect.bottom | cropRect.right;

	img_size = stride * height * 3 / 2;

	if (chromaInterleave == 0 && cropping == 0) {
		fwriten(dec->cmdl->dst_fd, (u8 *)buf, img_size);
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

int
decoder_start(struct decode *dec)
{
	DecHandle handle = dec->handle;
	DecOutputInfo outinfo = {0};
	DecParam decparam = {0};
	int rot_en = dec->cmdl->rot_en, rot_stride, fwidth, fheight;
	int rot_angle = dec->cmdl->rot_angle;
	int deblock_en = dec->cmdl->deblock_en;
	int dering_en = dec->cmdl->dering_en;
	FrameBuffer *deblock_fb = NULL;
	FrameBuffer *fb = dec->fb;
	struct frame_buf **pfbpool = dec->pfbpool;
	struct frame_buf *pfb = NULL;
	struct vpu_display *disp = dec->disp;
	int err, eos = 0, fill_end_bs = 0, decodefinish = 0;
	struct timeval tdec_begin,tdec_end, total_start, total_end;
	RetCode ret;
	int sec, usec;
	u32 yuv_addr, img_size;
	double tdec_time = 0, frame_id = 0, total_time=0;
	int decIndex = 0;
	int rotid = 0, dblkid = 0, mirror;
	int count = dec->cmdl->count;
	int totalNumofErrMbs = 0;
	int disp_clr_index = -1, actual_display_index = -1;

	if ((dec->cmdl->dst_scheme == PATH_V4L2) && (dec->cmdl->ipu_rot_en))
		rot_en = 0;

	/* deblock_en is zero on mx37 and mx51 since it is cleared in decode_open() function. */
	if (rot_en || dering_en) {
		rotid = dec->fbcount;
		if (deblock_en) {
			dblkid = dec->fbcount + dec->rot_buf_count;
		}
	} else if (deblock_en) {
		dblkid = dec->fbcount;
	}

	decparam.dispReorderBuf = 0;

	decparam.prescanEnable = dec->cmdl->prescan;
	decparam.prescanMode = 0;

	decparam.skipframeMode = 0;
	decparam.skipframeNum = 0;
	/*
	 * once iframeSearchEnable is enabled, prescanEnable, prescanMode
	 * and skipframeMode options are ignored.
	 */
	decparam.iframeSearchEnable = 0;

	decparam.extParam.userDataEnable = 0;

	decparam.extParam.mbParamEnable = 0;
	decparam.extParam.mvReportEnable = 0;
	decparam.extParam.frameBufStatEnable = 0;

	fwidth = ((dec->picwidth + 15) & ~15);
	fheight = ((dec->picheight + 15) & ~15);

	if (rot_en || dering_en) {
		/*
		 * VPU is setting the rotation angle by counter-clockwise.
		 * We convert it to clockwise, which is consistent with V4L2
		 * rotation angle strategy.
		 */
		if (rot_angle == 90 || rot_angle == 270)
			rot_angle = 360 - rot_angle;
		vpu_DecGiveCommand(handle, SET_ROTATION_ANGLE,
					&rot_angle);

		mirror = dec->cmdl->mirror;
		vpu_DecGiveCommand(handle, SET_MIRROR_DIRECTION,
					&mirror);

		rot_stride = (rot_angle == 90 || rot_angle == 270) ?
					fheight : fwidth;

		vpu_DecGiveCommand(handle, SET_ROTATOR_STRIDE, &rot_stride);
	}

	if (deblock_en) {
		deblock_fb = &fb[dblkid];
	}
	
	if (dec->cmdl->dst_scheme == PATH_V4L2) {
		img_size = dec->stride * dec->picheight;
	} else {
		img_size = dec->picwidth * dec->picheight * 3 / 2;
		if (deblock_en) {
			pfb = pfbpool[dblkid];
			deblock_fb->bufY = pfb->addrY;
			deblock_fb->bufCb = pfb->addrCb;
			deblock_fb->bufCr = pfb->addrCr;
		}
	}

	gettimeofday(&total_start, NULL);

	while (1) {
		
		if (rot_en || dering_en) {
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
		
		if (deblock_en) {
			ret = vpu_DecGiveCommand(handle, DEC_SET_DEBLOCK_OUTPUT,
						(void *)deblock_fb);
			if (ret != RETCODE_SUCCESS) {
				err_msg("Failed to set deblocking output\n");
				return -1;
			}
		}

		gettimeofday(&tdec_begin, NULL);
		ret = vpu_DecStartOneFrame(handle, &decparam);
		if (ret != RETCODE_SUCCESS) {
			err_msg("DecStartOneFrame failed\n");
			return -1;
		}

		while (vpu_IsBusy()) {
			err = dec_fill_bsbuffer(handle, dec->cmdl,
				      dec->virt_bsbuf_addr,
				      (dec->virt_bsbuf_addr + STREAM_BUF_SIZE),
				      dec->phy_bsbuf_addr, STREAM_FILL_SIZE,
				      &eos, &fill_end_bs);

			if (err < 0) {
				err_msg("dec_fill_bsbuffer failed\n");
				return -1;
			}

			if (!err) {
				vpu_WaitForInt(500);
			}
		}

		gettimeofday(&tdec_end, NULL);
		sec = tdec_end.tv_sec - tdec_begin.tv_sec;
		usec = tdec_end.tv_usec - tdec_begin.tv_usec;

		if (usec < 0) {
			sec--;
			usec = usec + 1000000;
		}

		tdec_time += (sec * 1000000) + usec;

		ret = vpu_DecGetOutputInfo(handle, &outinfo);
		dprintf(4, "frame_id = %d\n", (int)frame_id);
		if (ret != RETCODE_SUCCESS) {
			err_msg("vpu_DecGetOutputInfo failed Err code is %d\n"
				"\tframe_id = %d\n", ret, (int)frame_id);
			return -1;
		}

		if (outinfo.decodingSuccess == 0) {
			warn_msg("Incomplete finish of decoding process.\n"
				"\tframe_id = %d\n", (int)frame_id);
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

		/* Frame Buffer Status */
		if (decparam.extParam.frameBufStatEnable &&
			outinfo.outputExtData.frameBufDataSize) {
			int size;
			u8 *frameBufferStatBuf;
			size = (outinfo.outputExtData.frameBufDataSize+7)*8/8;

			frameBufferStatBuf = malloc(size);
			memset(frameBufferStatBuf, 0, size);
			memcpy(frameBufferStatBuf, (void *)outinfo.outputExtData.frameBufStatDataAddr, size);
			SaveFrameBufStat(frameBufferStatBuf, outinfo.outputExtData.frameBufDataSize);
			free(frameBufferStatBuf);
		}

		/* Mb Param */
		if (outinfo.indexFrameDecoded >= 0 &&
			decparam.extParam.mbParamEnable &&
			outinfo.outputExtData.mbParamDataSize) {
			int size;
			u8 *mbDataBuf;
			int mbNumX = dec->picwidth / 16;

			size = (outinfo.outputExtData.mbParamDataSize + 7) / 8 * 8;
			mbDataBuf = malloc(size);
			memset(mbDataBuf, 0, size);
			memcpy(mbDataBuf, (void *)outinfo.outputExtData.mbParamDataAddr, size);
			SaveMB_Para(mbDataBuf, outinfo.outputExtData.mbParamDataSize,
						mbNumX, decIndex);
			free(mbDataBuf);
		}

		/* Motion Vector */
		if (outinfo.indexFrameDecoded >= 0 &&
			decparam.extParam.mvReportEnable &&
			outinfo.outputExtData.mvDataSize) {
			int size;
			u8 *mvDataBuf;
			int mbNumX = dec->picwidth / 16;
			size = (outinfo.outputExtData.mvDataSize+7)/8*8;
			size = size*outinfo.outputExtData.mvNumPerMb*4;
			mvDataBuf = malloc(size);
			memset(mvDataBuf, 0, size);
			memcpy(mvDataBuf, (void *)outinfo.outputExtData.mvDataAddr, size);
			SaveMvPara(mvDataBuf, outinfo.outputExtData.mvDataSize,
				   outinfo.outputExtData.mvNumPerMb,
				   mbNumX, decIndex);
			free(mvDataBuf);
		}

		decIndex++;

		if (outinfo.indexFrameDisplay == -1)
			decodefinish = 1;
		else if ((outinfo.indexFrameDisplay > dec->fbcount) &&
				(outinfo.prescanresult != 0))
			decodefinish = 1;

		if (decodefinish)
			break;

		if ((outinfo.prescanresult == 0) &&
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
			warn_msg("VPU doesn't have picture to be displayed.\n"
				"\toutinfo.indexFrameDisplay = %d\n",
						outinfo.indexFrameDisplay);
			if (disp_clr_index >= 0) {
				err = vpu_DecClrDispFlag(handle, disp_clr_index);
				if (err)
					err_msg("vpu_DecClrDispFlag failed Error code"
							" %d\n", err);
			}
			disp_clr_index = outinfo.indexFrameDisplay;
			continue;
		}

		if (rot_en || dering_en)
			actual_display_index = rotid;
		else
			actual_display_index = outinfo.indexFrameDisplay;

		if (dec->cmdl->dst_scheme == PATH_V4L2) {
			if (deblock_en) {
				deblock_fb->bufY =
					disp->buffers[disp->buf.index]->offset;
				deblock_fb->bufCb = deblock_fb->bufY + img_size;
				deblock_fb->bufCr = deblock_fb->bufCb +
							(img_size >> 2);
			}

			err = v4l_put_data(disp, actual_display_index);

			if (err)
				return -1;

			if (disp_clr_index >= 0) {
				err = vpu_DecClrDispFlag(handle, disp_clr_index);
				if (err)
					err_msg("vpu_DecClrDispFlag failed Error code"
							" %d\n", err);
			}
			if (rot_en || dering_en) {
				disp_clr_index = outinfo.indexFrameDisplay;
				if (disp->buf.index != -1)
                                        rotid = disp->buf.index; /* de-queued buffer as next rotation buffer */
                                else
                                        rotid++;
			}
			else
				disp_clr_index = disp->buf.index;
		} else {
			pfb = pfbpool[actual_display_index];

			yuv_addr = pfb->addrY + pfb->desc.virt_uaddr -
					pfb->desc.phy_addr;

			if (rot_en) {
				Rect rotCrop;
				swapCropRect(dec, &rotCrop);
				write_to_file(dec, (u8 *)yuv_addr, rotCrop);
			} else {
				write_to_file(dec, (u8 *)yuv_addr,
							dec->picCropRect);
			}

			if (disp_clr_index >= 0) {
				err = vpu_DecClrDispFlag(handle,disp_clr_index);
				if (err)
					err_msg("vpu_DecClrDispFlag failed Error code"
						" %d\n", err);
			}
			disp_clr_index = outinfo.indexFrameDisplay;
		}

		if (outinfo.numOfErrMBs[0]) {
			totalNumofErrMbs += outinfo.numOfErrMBs[0];
			info_msg("Num of Error Mbs : %d, in Frame : %d \n",
					outinfo.numOfErrMBs[0], (int)frame_id);
		}

		frame_id++;
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
	}

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
	info_msg("dec fps = %.2f\n", (frame_id / (tdec_time / 1000000)));
	info_msg("total fps= %.2f \n",(frame_id / (total_time / 1000000)));
	return 0;
}

void
decoder_free_framebuffer(struct decode *dec)
{
	int i, totalfb;
	vpu_mem_desc *mvcol_md = dec->mvcol_memdesc;
	int deblock_en = dec->cmdl->deblock_en;

	totalfb = dec->fbcount + dec->extrafb;

	if (dec->cmdl->dst_scheme == PATH_V4L2) {
		if (dec->disp) {
			v4l_display_close(dec->disp);
			dec->disp = NULL;
		}

		if (mvcol_md) {
			for (i = 0; i < totalfb; i++) {
				if (mvcol_md[i].phy_addr)
					IOFreePhyMem(&mvcol_md[i]);
			}
			if (dec->mvcol_memdesc) {
				free(dec->mvcol_memdesc);
				dec->mvcol_memdesc = NULL;
			}
		}
	}

	/* deblock_en is zero on mx37 and mx51 since it is cleared in decode_open() function. */
	if ((dec->cmdl->dst_scheme != PATH_V4L2) ||
	    ((dec->cmdl->dst_scheme == PATH_V4L2) && deblock_en)) {
		for (i = 0; i < totalfb; i++) {
			framebuf_free(dec->pfbpool[i]);
		}
	}

	if (dec->fb) {
		free(dec->fb);
		dec->fb = NULL;
	}
	if (dec->pfbpool) {
		free(dec->pfbpool);
		dec->pfbpool = NULL;
	}
	return;
}

int
decoder_allocate_framebuffer(struct decode *dec)
{
	DecBufInfo bufinfo;
	int i, fbcount = dec->fbcount, totalfb, img_size;
       	int dst_scheme = dec->cmdl->dst_scheme, rot_en = dec->cmdl->rot_en;
	int deblock_en = dec->cmdl->deblock_en;
	int dering_en = dec->cmdl->dering_en;
	struct rot rotation = {0};
	RetCode ret;
	DecHandle handle = dec->handle;
	FrameBuffer *fb;
	struct frame_buf **pfbpool;
	struct vpu_display *disp = NULL;
	int stride;
	vpu_mem_desc *mvcol_md = NULL;
	Rect rotCrop;

	if ((dec->cmdl->dst_scheme == PATH_V4L2) && (dec->cmdl->ipu_rot_en))
		rot_en = 0;

	if (rot_en || dering_en) {
		/*
		 * At least 1 extra fb for rotation(or dering) is needed, two extrafb
		 * are allocated for rotation if path is V4L,then we can delay 1 frame
		 * de-queue from v4l queue to improve performance.
		 */
		dec->rot_buf_count = (dec->cmdl->dst_scheme == PATH_V4L2) ? 2 : 1;
		dec->extrafb += dec->rot_buf_count;
	}

	/*
	 * 1 extra fb for deblocking on MX32, no need extrafb for blocking on MX37 and MX51
	 * dec->cmdl->deblock_en has been cleared to zero after set it to oparam.mp4DeblkEnable
	 * in decoder_open() function on MX37 and MX51.
	 */
	if (deblock_en) {
		dec->extrafb++;
	}

	totalfb = fbcount + dec->extrafb;

	fb = dec->fb = calloc(totalfb, sizeof(FrameBuffer));
	if (fb == NULL) {
		err_msg("Failed to allocate fb\n");
		return -1;
	}

	pfbpool = dec->pfbpool = calloc(totalfb, sizeof(struct frame_buf *));
	if (pfbpool == NULL) {
		err_msg("Failed to allocate pfbpool\n");
		free(dec->fb);
		dec->fb = NULL;
		return -1;
	}
	
	if ((dst_scheme != PATH_V4L2) ||
	    ((dst_scheme == PATH_V4L2) && deblock_en)) {

		for (i = 0; i < totalfb; i++) {
			pfbpool[i] = framebuf_alloc(dec->cmdl->format, dec->stride,
						    dec->picheight);
			if (pfbpool[i] == NULL) {
				totalfb = i;
				goto err;
			}

			fb[i].bufY = pfbpool[i]->addrY;
			fb[i].bufCb = pfbpool[i]->addrCb;
			fb[i].bufCr = pfbpool[i]->addrCr;
			if (cpu_is_mx37() || cpu_is_mx51()) {
				fb[i].bufMvCol = pfbpool[i]->mvColBuf;
			}
		}
	}
	
	if (dst_scheme == PATH_V4L2) {
		rotation.rot_en = dec->cmdl->rot_en;
		rotation.rot_angle = dec->cmdl->rot_angle;

		if (dec->cmdl->ipu_rot_en) {
			rotation.rot_en = 0;
			rotation.ipu_rot_en = 1;
		}
		if (rotation.rot_en) {
			swapCropRect(dec, &rotCrop);
			disp = v4l_display_open(dec, totalfb, rotation, rotCrop);
		} else
			disp = v4l_display_open(dec, totalfb, rotation, dec->picCropRect);

		if (disp == NULL) {
			goto err;
		}

		if (deblock_en == 0) {
			img_size = dec->stride * dec->picheight;

			if (cpu_is_mx37() || cpu_is_mx51()) {
				mvcol_md = dec->mvcol_memdesc =
					calloc(totalfb, sizeof(vpu_mem_desc));
			}
			for (i = 0; i < totalfb; i++) {
				fb[i].bufY = disp->buffers[i]->offset;
				fb[i].bufCb = fb[i].bufY + img_size;
				fb[i].bufCr = fb[i].bufCb + (img_size >> 2);
				/* allocate MvCol buffer here */
				if (cpu_is_mx37() || cpu_is_mx51()) {
					memset(&mvcol_md[i], 0,
							sizeof(vpu_mem_desc));
					mvcol_md[i].size = img_size >> 2;
					ret = IOGetPhyMem(&mvcol_md[i]);
					if (ret) {
						err_msg("buf alloc failed\n");
						goto err1;
					}
					fb[i].bufMvCol = mvcol_md[i].phy_addr;
				}
			}
		}
	}

	stride = ((dec->stride + 15) & ~15);
	bufinfo.avcSliceBufInfo.sliceSaveBuffer = dec->phy_slice_buf;
	bufinfo.avcSliceBufInfo.sliceSaveBufferSize = dec->phy_slicebuf_size;
	
	ret = vpu_DecRegisterFrameBuffer(handle, fb, fbcount, stride, &bufinfo);
	if (ret != RETCODE_SUCCESS) {
		err_msg("Register frame buffer failed\n");
		goto err1;
	}

	dec->disp = disp;
	return 0;

err1:
	if (dst_scheme == PATH_V4L2) {
		v4l_display_close(disp);
		dec->disp = NULL;
	}

err:
	if ((dst_scheme != PATH_V4L2) ||
	   ((dst_scheme == PATH_V4L2) && deblock_en )) {
		for (i = 0; i < totalfb; i++) {
			framebuf_free(pfbpool[i]);
		}
	}

	free(dec->fb);
	free(dec->pfbpool);
	dec->fb = NULL;
	dec->pfbpool = NULL;
	return -1;
}

int
decoder_parse(struct decode *dec)
{
	DecInitialInfo initinfo = {0};
	DecHandle handle = dec->handle;
	RetCode ret;

	/* Parse bitstream and get width/height/framerate etc */
	vpu_DecSetEscSeqInit(handle, 1);
	ret = vpu_DecGetInitialInfo(handle, &initinfo);
	vpu_DecSetEscSeqInit(handle, 0);
	if (ret != RETCODE_SUCCESS) {
		err_msg("vpu_DecGetInitialInfo failed %d\n", ret);
		return -1;
	}

	if (initinfo.streamInfoObtained) {
		switch (dec->cmdl->format) {
		case STD_AVC:
			info_msg("H.264 Profile: %d Level: %d FrameMbsOnlyFlag: %d\n",
				initinfo.profile, initinfo.level, initinfo.interlace);

			if (initinfo.aspectRateInfo) {
				int aspect_ratio_idc;
				int sar_width, sar_height;

				if ((initinfo.aspectRateInfo >> 16) == 0) {
					aspect_ratio_idc = (initinfo.aspectRateInfo & 0xFF);
					info_msg("aspect_ratio_idc: %d\n", aspect_ratio_idc);
				} else {
					sar_width = (initinfo.aspectRateInfo >> 16);
					sar_height = (initinfo.aspectRateInfo & 0xFFFF);
					info_msg("sar_width: %d\nsar_height: %d",
						sar_width, sar_height);
				}
			} else {
				info_msg("Aspect Ratio is not present.\n");
			}

			break;
		case STD_VC1:
			if (initinfo.profile == 0)
				info_msg("VC1 Profile: Simple\n");
			else if (initinfo.profile == 1)
				info_msg("VC1 Profile: Main\n");
			else if (initinfo.profile == 2)
				info_msg("VC1 Profile: Advanced\n");

			info_msg("Level: %d Interlace: %d PSF: %d\n",
				initinfo.level, initinfo.interlace, initinfo.vc1_psf);

			if (initinfo.aspectRateInfo)
				info_msg("Aspect Ratio [X, Y]:[%3d, %3d]\n",
						(initinfo.aspectRateInfo >> 8) & 0xff,
						(initinfo.aspectRateInfo) & 0xff);
			else
				info_msg("Aspect Ratio is not present.\n");

			break;
		case STD_MPEG2:
			info_msg("Mpeg2 Profile: %d Level: %d Progressive Sequence Flag: %d\n",
				initinfo.profile, initinfo.level, initinfo.interlace);
			/*
			 * Profile: 3'b101: Simple, 3'b100: Main, 3'b011: SNR Scalable,
			 * 3'b10: Spatially Scalable, 3'b001: High
			 * Level: 4'b1010: Low, 4'b1000: Main, 4'b0110: High 1440, 4'b0100: High
			 */
			if (initinfo.aspectRateInfo)
				info_msg("Aspect Ratio Table index: %d\n", initinfo.aspectRateInfo);
			else
				info_msg("Aspect Ratio is not present.\n");
			break;

		case STD_MPEG4:
			info_msg("Mpeg4 Profile: %d Level: %d Interlaced: %d\n",
				initinfo.profile, initinfo.level, initinfo.interlace);
			/*
			 * Profile: 8'b00000000: SP, 8'b00010001: ASP
			 * Level: 4'b0000: L0, 4'b0001: L1, 4'b0010: L2, 4'b0011: L3, ...
			 * SP: 1/2/3/4a/5/6, ASP: 0/1/2/3/4/5
			 */
			if (initinfo.aspectRateInfo)
				info_msg("Aspect Ratio Table index: %d\n", initinfo.aspectRateInfo);
			else
				info_msg("Aspect Ratio is not present.\n");

			break;
		}
	}

	info_msg("Decoder: width = %d, height = %d, fps = %lu, count = %u\n",
			initinfo.picWidth, initinfo.picHeight,
			initinfo.frameRateInfo,
			initinfo.minFrameBufferCount);

	/*
	 * Information about H.264 decoder picture cropping rectangle which
	 * presents the offset of top-left point and bottom-right point from
	 * the origin of frame buffer.
	 *
	 * By using these four offset values, host application can easily
	 * detect the position of target output window. When display cropping
	 * is off, the cropping window size will be 0.
	 *
	 * This structure for cropping rectangles is only valid for H.264
	 * decoder case.
	 */
	info_msg("CROP left/top/right/bottom %lu %lu %lu %lu\n",
					initinfo.picCropRect.left,
					initinfo.picCropRect.top,
					initinfo.picCropRect.right,
					initinfo.picCropRect.bottom);

	/*
	 * We suggest to add two more buffers than minFrameBufferCount:
	 *
	 * vpu_DecClrDispFlag is used to control framebuffer whether can be
	 * used for decoder again. One framebuffer dequeue from IPU is delayed
	 * for performance improvement and one framebuffer is delayed for
	 * display flag clear.
	 *
	 * Performance is better when more buffers are used if IPU performance
	 * is bottleneck.
	 */
	dec->fbcount = initinfo.minFrameBufferCount + 2;
	dec->picwidth = ((initinfo.picWidth + 15) & ~15);
	dec->picheight = ((initinfo.picHeight + 15) & ~15);
	if ((dec->picwidth == 0) || (dec->picheight == 0))
		return -1;

	memcpy(&(dec->picCropRect), &(initinfo.picCropRect),
					sizeof(initinfo.picCropRect));

	/* worstSliceSize is in kilo-byte unit */
	dec->phy_slicebuf_size = initinfo.worstSliceSize * 1024;
	dec->stride = dec->picwidth;
	return 0;
}

int
decoder_open(struct decode *dec)
{
	RetCode ret;
	DecHandle handle = {0};
	DecOpenParam oparam = {0};

	oparam.bitstreamFormat = dec->cmdl->format;
	oparam.bitstreamBuffer = dec->phy_bsbuf_addr;
	oparam.bitstreamBufferSize = STREAM_BUF_SIZE;
	oparam.reorderEnable = dec->reorderEnable;
	oparam.mp4DeblkEnable = dec->cmdl->deblock_en;
	oparam.chromaInterleave = dec->cmdl->chromaInterleave;

	/*
	 * mp4 deblocking filtering is optional out-loop filtering for image
	 * quality. In other words, mpeg4 deblocking is post processing.
	 * So, host application need to allocate new frame buffer.
	 * - On MX27, VPU doesn't support mpeg4 out loop deblocking filtering.
	 * - On MX37 and MX51, VPU control the buffer internally and return one
	 *   more buffer after vpu_DecGetInitialInfo().
	 * - On MX32, host application need to set frame buffer externally via
	 *   the command DEC_SET_DEBLOCK_OUTPUT.
	 */
	if (oparam.mp4DeblkEnable == 1) {
		if (cpu_is_mx27()) {
			warn_msg("MP4 deblocking NOT supported on MX27 VPU.\n");
			oparam.mp4DeblkEnable = dec->cmdl->deblock_en = 0;
		} else if (!cpu_is_mx32()) {	/* do not need extra framebuf */
			dec->cmdl->deblock_en = 0;
		}
	}

	oparam.psSaveBuffer = dec->phy_ps_buf;
	oparam.psSaveBufferSize = PS_SAVE_SIZE;

	ret = vpu_DecOpen(&handle, &oparam);
	if (ret != RETCODE_SUCCESS) {
		err_msg("vpu_DecOpen failed\n");
		return -1;
	}

	dec->handle = handle;
	return 0;
}

void decoder_close(struct decode *dec)
{
	DecOutputInfo outinfo = {0};
	RetCode ret;

	ret = vpu_DecClose(dec->handle);
	if (ret == RETCODE_FRAME_NOT_COMPLETE) {
		vpu_DecGetOutputInfo(dec->handle, &outinfo);
		ret = vpu_DecClose(dec->handle);
		if (ret != RETCODE_SUCCESS)
			err_msg("vpu_DecClose failed\n");
	}
}

int
decode_test(void *arg)
{
	struct cmd_line *cmdl = (struct cmd_line *)arg;
	vpu_mem_desc mem_desc = {0};
	vpu_mem_desc ps_mem_desc = {0};
	vpu_mem_desc slice_mem_desc = {0};
	struct decode *dec;
	int ret, eos = 0, fill_end_bs = 0, fillsize = 0;

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

	/* allocate frame buffers */
	ret = decoder_allocate_framebuffer(dec);
	if (ret)
		goto err1;

	/* start decoding */
	ret = decoder_start(dec);
err1:
	decoder_close(dec);

	/* free the frame buffers */
	decoder_free_framebuffer(dec);
err:
	if (cmdl->format == STD_AVC) {
		IOFreePhyMem(&slice_mem_desc);
		IOFreePhyMem(&ps_mem_desc);
	}

	IOFreeVirtMem(&mem_desc);
	IOFreePhyMem(&mem_desc);
	free(dec);
	return ret;
}

