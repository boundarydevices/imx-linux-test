/*
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * Take into account a different stride when writing to file
 */
static void
write_to_file(struct decode *dec, u8 *buf)
{
	int i;
	int Y, Cb, Cr;
	int width = dec->picwidth;
	int height = dec->picheight;
	int stride = dec->stride;

	Y = (int)buf;
	Cb = Y + (stride * height);
	Cr = Cb +  (stride / 2  * height / 2);

	for (i = 0; i < height; i++) {
		fwriten(dec->cmdl->dst_fd, (u8 *)Y, width);
		Y += stride;
	}

	for (i = 0; i < height / 2; i++) {
		fwriten(dec->cmdl->dst_fd, (u8 *)Cb, width / 2);
		Cb += stride / 2;
	}

	for (i = 0; i < height / 2; i++) {
		fwriten(dec->cmdl->dst_fd, (u8 *)Cr, width / 2);
		Cr += stride / 2;
	}
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
write_to_file2(struct decode *dec, u8 *buf, Rect cropRect)
{
	int height = dec->picheight;
	int stride = dec->stride;
	int chromaInterleave = dec->chromaInterleave;
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
	FrameBuffer *rot_fb = NULL;
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
	float tdec_time = 0, frame_id = 0, total_time=0;
	int rotid = 0, dblkid = 0, mirror;
	int count = dec->cmdl->count;
	int totalNumofErrMbs = 0;
	int disp_clr_index = -1;

	if ((dec->cmdl->dst_scheme == PATH_V4L2) && (dec->cmdl->ipu_rot_en))
		rot_en = 0;

	if (rot_en || dering_en) {
		rotid = dec->fbcount;
		if (deblock_en) {
			dblkid = dec->fbcount + 1;
		}
	} else if (deblock_en) {
		dblkid = dec->fbcount;
	}

	decparam.dispReorderBuf = 0;

	decparam.prescanEnable = 0;
	decparam.prescanMode = 0;

	decparam.skipframeMode = 0;
	decparam.skipframeNum = 0;
	/*
	 * once iframeSearchEnable is enabled, prescanEnable, prescanMode
	 * and skipframeMode options are ignored.
	 */
	decparam.iframeSearchEnable = 0;

	fwidth = ((dec->picwidth + 15) & ~15);
	fheight = ((dec->picheight + 15) & ~15);

	if (rot_en || dering_en) {
		rot_fb = &fb[rotid];

		lock(dec->cmdl);
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
		unlock(dec->cmdl);
	}

	if (deblock_en) {
		deblock_fb = &fb[dblkid];
	}
	
	if (dec->cmdl->dst_scheme == PATH_V4L2) {
		img_size = dec->stride * dec->picheight;
	} else {
		img_size = dec->picwidth * dec->picheight * 3 / 2;
		if (rot_en || dering_en) {
			pfb = pfbpool[rotid];
			rot_fb->bufY = pfb->addrY;
			rot_fb->bufCb = pfb->addrCb;
			rot_fb->bufCr = pfb->addrCr;
		} else if (deblock_en) {
			pfb = pfbpool[dblkid];
			deblock_fb->bufY = pfb->addrY;
			deblock_fb->bufCb = pfb->addrCb;
			deblock_fb->bufCr = pfb->addrCr;
		}
	}

	gettimeofday(&total_start, NULL);

	while (1) {
		lock(dec->cmdl);
		
		if (rot_en || dering_en) {
			vpu_DecGiveCommand(handle, SET_ROTATOR_OUTPUT,
						(void *)rot_fb);
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
				unlock(dec->cmdl);
				return -1;
			}
		}

		gettimeofday(&tdec_begin, NULL);
		ret = vpu_DecStartOneFrame(handle, &decparam);
		if (ret != RETCODE_SUCCESS) {
			err_msg("DecStartOneFrame failed\n");
			unlock(dec->cmdl);
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
				unlock(dec->cmdl);
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
		unlock(dec->cmdl);
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
			continue;
		}

		if (dec->cmdl->dst_scheme == PATH_V4L2) {
			if (rot_en || dering_en) {
				rot_fb->bufY =
					disp->buffers[outinfo.indexFrameDisplay]->offset;
				rot_fb->bufCb = rot_fb->bufY + img_size;
				rot_fb->bufCr = rot_fb->bufCb +
						(img_size >> 2);
			} else if (deblock_en) {
				deblock_fb->bufY =
					disp->buffers[disp->buf.index]->offset;
				deblock_fb->bufCb = deblock_fb->bufY + img_size;
				deblock_fb->bufCr = deblock_fb->bufCb +
							(img_size >> 2);
			}

			err = v4l_put_data(disp, outinfo.indexFrameDisplay);
			if (err)
				return -1;

			if (disp_clr_index != -1) {
				err = vpu_DecClrDispFlag(handle, disp_clr_index);
				if (err)
					err_msg("vpu_DecClrDispFlag failed Error code"
							" %d\n", err);
                        }
                        disp_clr_index = disp->buf.index;
		} else {
			pfb = pfbpool[outinfo.indexFrameDisplay];

			yuv_addr = pfb->addrY + pfb->desc.virt_uaddr -
					pfb->desc.phy_addr;


			if (cpu_is_mxc30031()) {
				write_to_file(dec, (u8 *)yuv_addr);
			} else {
				if (rot_en) {
					Rect rotCrop;
					swapCropRect(dec, &rotCrop);
					write_to_file2(dec, (u8 *)yuv_addr,
							rotCrop);
				} else {
					write_to_file2(dec, (u8 *)yuv_addr,
							dec->picCropRect);
				}
			}

			if (disp_clr_index != -1) {
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
	int rot_en = dec->cmdl->rot_en;
	int deblock_en = dec->cmdl->deblock_en;
	int dering_en = dec->cmdl->dering_en;

	if ((dec->cmdl->dst_scheme == PATH_V4L2) && (dec->cmdl->ipu_rot_en))
		rot_en = 0;

	if (dec->cmdl->dst_scheme == PATH_V4L2) {
		v4l_display_close(dec->disp);

		if ((rot_en == 0) && (deblock_en == 0) && (dering_en == 0)) {
			if (cpu_is_mx37() || cpu_is_mx51()) {
				for (i = 0; i < dec->fbcount; i++) {
					IOFreePhyMem(&mvcol_md[i]);
				}
				free(mvcol_md);
			}
		}
	}

	if ((dec->cmdl->dst_scheme != PATH_V4L2) ||
			((dec->cmdl->dst_scheme == PATH_V4L2) &&
			 (rot_en || deblock_en ||
			 ((cpu_is_mx37() || cpu_is_mx51()) && dering_en)))) {
		totalfb = dec->fbcount + dec->extrafb;
		for (i = 0; i < totalfb; i++) {
			framebuf_free(dec->pfbpool[i]);
		}
	}

	free(dec->fb);
	free(dec->pfbpool);
	return;
}

int
decoder_allocate_framebuffer(struct decode *dec)
{
	DecBufInfo bufinfo;
	int i, fbcount = dec->fbcount, extrafb = 0, totalfb, img_size;
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

	if ((dec->cmdl->dst_scheme == PATH_V4L2) && (dec->cmdl->ipu_rot_en))
		rot_en = 0;

	/* 1 extra fb for rotation(or dering) */
	if (rot_en || dering_en) {
		extrafb++;
		dec->extrafb++;
	}

	/* 1 extra fb for deblocking */
	if (deblock_en) {
		extrafb++;
		dec->extrafb++;
	}

	totalfb = fbcount + extrafb;

	fb = dec->fb = calloc(totalfb, sizeof(FrameBuffer));
	if (fb == NULL) {
		err_msg("Failed to allocate fb\n");
		return -1;
	}

	pfbpool = dec->pfbpool = calloc(totalfb, sizeof(struct frame_buf *));
	if (pfbpool == NULL) {
		err_msg("Failed to allocate pfbpool\n");
		free(fb);
		return -1;
	}
	
	if ((dst_scheme != PATH_V4L2) ||
		((dst_scheme == PATH_V4L2) && (rot_en || deblock_en ||
			((cpu_is_mx37()||cpu_is_mx51()) && dering_en)))) {

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
		disp = v4l_display_open(dec, fbcount, rotation);
		if (disp == NULL) {
			goto err;
		}

		if ((rot_en == 0) && (deblock_en == 0) && (dering_en == 0)) {
			img_size = dec->stride * dec->picheight;

			if (cpu_is_mx37() || cpu_is_mx51()) {
				mvcol_md = dec->mvcol_memdesc =
					calloc(fbcount, sizeof(vpu_mem_desc));
			}
			for (i = 0; i < fbcount; i++) {
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
	bufinfo.avcSliceBufInfo.sliceSaveBufferSize = SLICE_SAVE_SIZE;
	
	lock(dec->cmdl);
	ret = vpu_DecRegisterFrameBuffer(handle, fb, fbcount, stride, &bufinfo);
	unlock(dec->cmdl);
	if (ret != RETCODE_SUCCESS) {
		err_msg("Register frame buffer failed\n");
		goto err1;
	}

	dec->disp = disp;
	return 0;

err1:
	if (dst_scheme == PATH_V4L2) {
		v4l_display_close(disp);
	}

err:
	if ((dst_scheme != PATH_V4L2) ||
		((dst_scheme == PATH_V4L2) && (rot_en || deblock_en ||
			((cpu_is_mx37() || cpu_is_mx51()) && dering_en)))) {
		for (i = 0; i < totalfb; i++) {
			framebuf_free(pfbpool[i]);
		}
	}

	free(fb);
	free(pfbpool);
	return -1;
}

static void
calculate_stride(struct decode *dec)
{
	if (cpu_is_mxc30031()) {
		if (dec->picwidth <= 128) {
			dec->stride = 128;
		} else if (dec->picwidth <= 256) {
			dec->stride = 256;
		} else if (dec->picwidth <= 512) {
			dec->stride = 512;
		} else if (dec->picwidth <= 1024) {
			dec->stride = 1024;
		}
	} else {
		dec->stride = dec->picwidth;
	}
}

int
decoder_parse(struct decode *dec)
{
	DecInitialInfo initinfo = {0};
	DecHandle handle = dec->handle;
	RetCode ret;

	/* Parse bitstream and get width/height/framerate etc */
	lock(dec->cmdl);
	vpu_DecSetEscSeqInit(handle, 1);
	ret = vpu_DecGetInitialInfo(handle, &initinfo);
	vpu_DecSetEscSeqInit(handle, 0);
	unlock(dec->cmdl);
	if (ret != RETCODE_SUCCESS) {
		err_msg("vpu_DecGetInitialInfo failed %d\n", ret);
		return -1;
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

	 * Performance is better when more buffers are used if IPU performance
	 * is bottleneck.
	 */

	dec->fbcount = initinfo.minFrameBufferCount+2;
	dec->picwidth = ((initinfo.picWidth + 15) & ~15);
	dec->picheight = ((initinfo.picHeight + 15) & ~15);
	if ((dec->picwidth == 0) || (dec->picheight == 0))
		return -1;

	memcpy(&(dec->picCropRect), &(initinfo.picCropRect),
					sizeof(initinfo.picCropRect));

	calculate_stride(dec);
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
	oparam.mp4DeblkEnable = dec->cmdl->deblock_en;
	oparam.reorderEnable = dec->reorderEnable;

	oparam.psSaveBuffer = dec->phy_ps_buf;
	oparam.psSaveBufferSize = PS_SAVE_SIZE;

	oparam.chromaInterleave = dec->chromaInterleave;

	ret = vpu_DecOpen(&handle, &oparam);
	if (ret != RETCODE_SUCCESS) {
		err_msg("vpu_DecOpen failed\n");
		return -1;
	}

	dec->handle = handle;
	return 0;
}

int
decode_test(void *arg)
{
	struct cmd_line *cmdl = (struct cmd_line *)arg;
	vpu_mem_desc mem_desc = {0};
	vpu_mem_desc ps_mem_desc = {0};
	vpu_mem_desc slice_mem_desc = {0};
	struct decode *dec;
	int ret, eos = 0, fill_end_bs = 0;

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

	dec->chromaInterleave = 0;
	dec->reorderEnable = 1;
	dec->cmdl = cmdl;

	if (cmdl->format == STD_AVC) {
		ps_mem_desc.size = PS_SAVE_SIZE;
		ret = IOGetPhyMem(&ps_mem_desc);
		if (ret) {
			err_msg("Unable to obtain physical ps save mem\n");
			goto err;
		}

		slice_mem_desc.size = SLICE_SAVE_SIZE;
		ret = IOGetPhyMem(&slice_mem_desc);
		if (ret) {
			err_msg("Unable to obtain physical slice save mem\n");
			IOFreePhyMem(&ps_mem_desc);
			goto err;
		}

		dec->phy_ps_buf = ps_mem_desc.phy_addr;
		dec->phy_slice_buf = slice_mem_desc.phy_addr;
	}

	/* open decoder */
	ret = decoder_open(dec);
	if (ret)
		goto err;

	cmdl->complete = 1;
	ret = dec_fill_bsbuffer(dec->handle, cmdl,
			dec->virt_bsbuf_addr,
		        (dec->virt_bsbuf_addr + STREAM_BUF_SIZE),
			dec->phy_bsbuf_addr, 0, &eos, &fill_end_bs);
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

	/* allocate frame buffers */
	ret = decoder_allocate_framebuffer(dec);
	if (ret)
		goto err1;

	/* start decoding */
	ret = decoder_start(dec);

	/* free the frame buffers */
	decoder_free_framebuffer(dec);

err1:
	if (cmdl->format == STD_AVC) {
		IOFreeVirtMem(&slice_mem_desc);
		IOFreePhyMem(&slice_mem_desc);
		IOFreeVirtMem(&ps_mem_desc);
		IOFreePhyMem(&ps_mem_desc);
	}

	lock(cmdl);
	vpu_DecClose(dec->handle);
	unlock(cmdl);
err:
	IOFreeVirtMem(&mem_desc);
	IOFreePhyMem(&mem_desc);
	free(dec);
	return ret;
}

