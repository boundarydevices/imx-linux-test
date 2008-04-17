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

/* Fill the bitstream ring buffer
 *
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
		printf("vpu_DecGetBitstreamBuffer failed\n");
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
			printf("vpu_DecUpdateBitstreamBuffer failed\n");
			return -1;
		}
		*fill_end_bs = 0;
	} else {
		if (!*fill_end_bs) {
			ret = vpu_DecUpdateBitstreamBuffer(handle,
					STREAM_END_SIZE);
			if (ret != RETCODE_SUCCESS) {
				printf("vpu_DecUpdateBitstreamBuffer failed\n");
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

int
decoder_start(struct decode *dec)
{
	DecHandle handle = dec->handle;
	DecOutputInfo outinfo = {0};
	DecParam decparam = {0};
	int rot_en = dec->cmdl->rot_en, rot_stride, fwidth, fheight;
	int mp4dblk_en = dec->cmdl->mp4dblk_en;
	int dering_en = dec->cmdl->dering_en;
	FrameBuffer *rot_fb = NULL;
	FrameBuffer *mp4dblk_fb = NULL;
	FrameBuffer *fb = dec->fb;
	struct frame_buf **pfbpool = dec->pfbpool;
	struct frame_buf *pfb = NULL;
	struct vpu_display *disp = dec->disp;
	int err, eos = 0, fill_end_bs = 0, decodefinish = 0;
	struct timeval tbegin, tend;
	RetCode ret;
	int sec, usec;
	u32 yuv_addr, img_size;
	float total_time = 0, frame_id = 0;
	int rotid = 0, dblkid = 0, mirror;
	int count = dec->cmdl->count;
	int totalNumofErrMbs = 0;

	if (rot_en || dering_en) {
		rotid = dec->fbcount;
		if (mp4dblk_en) {
			dblkid = dec->fbcount + 1;
		}
	} else if (mp4dblk_en) {
		dblkid = dec->fbcount;
	}
	
	decparam.prescanEnable = 1;

	fwidth = ((dec->picwidth + 15) & ~15);
	fheight = ((dec->picheight + 15) & ~15);

	if (rot_en || dering_en) {
		rot_fb = &fb[rotid];

		lock(dec->cmdl);
		vpu_DecGiveCommand(handle, SET_ROTATION_ANGLE,
					&dec->cmdl->rot_angle);

		mirror = dec->cmdl->mirror;
		vpu_DecGiveCommand(handle, SET_MIRROR_DIRECTION,
					&mirror);

		rot_stride = (dec->cmdl->rot_angle == 90 ||
				dec->cmdl->rot_angle == 270) ? fheight :
				fwidth;

		vpu_DecGiveCommand(handle, SET_ROTATOR_STRIDE, &rot_stride);
		unlock(dec->cmdl);
	}

	if (mp4dblk_en) {
		mp4dblk_fb = &fb[dblkid];
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
		} else if (mp4dblk_en) {
			pfb = pfbpool[dblkid];
			mp4dblk_fb->bufY = pfb->addrY;
			mp4dblk_fb->bufCb = pfb->addrCb;
			mp4dblk_fb->bufCr = pfb->addrCr;
		}
	}
	
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
		
		if (mp4dblk_en) {
			ret = vpu_DecGiveCommand(handle, DEC_SET_DEBLOCK_OUTPUT,
						(void *)mp4dblk_fb);
			if (ret != RETCODE_SUCCESS) {
				printf("Failed to set deblocking output\n");
				unlock(dec->cmdl);
				return -1;
			}
		}
		
		ret = vpu_DecStartOneFrame(handle, &decparam);
		if (ret != RETCODE_SUCCESS) {
			printf("DecStartOneFrame failed\n");
			unlock(dec->cmdl);
			return -1;
		}

		gettimeofday(&tbegin, NULL);
		while (vpu_IsBusy()) {
			err = dec_fill_bsbuffer(handle, dec->cmdl,
				      dec->virt_bsbuf_addr,
				      (dec->virt_bsbuf_addr + STREAM_BUF_SIZE),
				      dec->phy_bsbuf_addr, STREAM_FILL_SIZE,
				      &eos, &fill_end_bs);
			
			if (err < 0) {
				printf("dec_fill_bsbuffer failed\n");
				unlock(dec->cmdl);
				return -1;
			}

			if (!err) {
				vpu_WaitForInt(500);
			}
		}

		gettimeofday(&tend, NULL);
		sec = tend.tv_sec - tbegin.tv_sec;
		usec = tend.tv_usec - tbegin.tv_usec;

		if (usec < 0) {
			sec--;
			usec = usec + 1000000;
		}

		total_time += (sec * 1000000) + usec;

		ret = vpu_DecGetOutputInfo(handle, &outinfo);
		unlock(dec->cmdl);
		if (ret == RETCODE_FAILURE) {
			frame_id++;
			continue;
		} else if (ret != RETCODE_SUCCESS) {
			printf("vpu_DecGetOutputInfo failed\n");
			return -1;
		}
		
		if (outinfo.notSufficientPsBuffer) {
			printf("PS Buffer overflow\n");
			return -1;
		}
		
		if (outinfo.notSufficientSliceBuffer) {
			printf("Slice Buffer overflow\n");
			return -1;
		}
				
		if (outinfo.indexFrameDisplay == -1)
			decodefinish = 1;
		else if ((outinfo.indexFrameDisplay > dec->fbcount) &&
				(outinfo.prescanresult != 0))
			decodefinish = 1;

		if (decodefinish)
			break;
		
		if (outinfo.prescanresult == 0) {
			if (eos) {
				break;
			} else {
				int fillsize = 0;

				if (dec->cmdl->src_scheme == PATH_NET)
					fillsize = 1000;
				else
					printf("Prescan: not enough bs data\n");

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
					printf("dec_fill_bsbuffer failed\n");
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
				(outinfo.indexFrameDisplay == -2))
			continue;

		if (dec->cmdl->dst_scheme == PATH_V4L2) {
			if (rot_en || dering_en) {
				rot_fb->bufY =
					disp->buffers[disp->buf.index]->offset;
				rot_fb->bufCb = rot_fb->bufY + img_size;
				rot_fb->bufCr = rot_fb->bufCb +
						(img_size >> 2);
			} else if (mp4dblk_en) {
				mp4dblk_fb->bufY =
					disp->buffers[disp->buf.index]->offset;
				mp4dblk_fb->bufCb = mp4dblk_fb->bufY + img_size;
				mp4dblk_fb->bufCr = mp4dblk_fb->bufCb +
							(img_size >> 2);
			}
			
			err = v4l_put_data(disp);
			if (err)
				return -1;

		} else {
			if (rot_en == 0 && mp4dblk_en == 0 && dering_en == 0) {
				pfb = pfbpool[outinfo.indexFrameDisplay];
			}
			
			yuv_addr = pfb->addrY + pfb->desc.virt_uaddr -
					pfb->desc.phy_addr;
		
			
			if (platform_is_mxc30031()) {
				write_to_file(dec, (u8 *)yuv_addr);
			} else {
				fwriten(dec->cmdl->dst_fd, (u8 *)yuv_addr,
						img_size);
			}
		}
		
		if (outinfo.numOfErrMBs) {
			totalNumofErrMbs += outinfo.numOfErrMBs;
			printf("Num of Error Mbs : %d, in Frame : %d \n",
					outinfo.numOfErrMBs, (int)frame_id);
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
				printf("dec_fill_bsbuffer failed\n");
				return -1;
			}
		}
	}

	if (totalNumofErrMbs) {
		printf("Total Num of Error MBs : %d\n", totalNumofErrMbs);
	}

	printf("%d frames took %d microseconds\n", (int)frame_id,
						(int)total_time);
	printf("fps = %.2f\n", (frame_id / (total_time / 1000000)));
	return 0;
}

void
decoder_free_framebuffer(struct decode *dec)
{
	int i, totalfb;
	vpu_mem_desc *mvcol_md = dec->mvcol_memdesc;
       	int rot_en = dec->cmdl->rot_en;
	int mp4dblk_en = dec->cmdl->mp4dblk_en;
	int dering_en = dec->cmdl->dering_en;

	if (dec->cmdl->dst_scheme == PATH_V4L2) {
		v4l_display_close(dec->disp);

		if ((rot_en == 0) && (mp4dblk_en == 0) && (dering_en == 0)) {
			if (platform_is_mx37()) {
				for (i = 0; i < dec->fbcount; i++) {
					IOFreePhyMem(&mvcol_md[i]);
				}
				free(mvcol_md);
			}
		}
	}

	if ((dec->cmdl->dst_scheme != PATH_V4L2) ||
			((dec->cmdl->dst_scheme == PATH_V4L2) &&
			 (dec->cmdl->rot_en || dec->cmdl->mp4dblk_en ||
			 (platform_is_mx37() && dec->cmdl->dering_en)))) {
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
	int mp4dblk_en = dec->cmdl->mp4dblk_en;
	int dering_en = dec->cmdl->dering_en;
	struct rot rotation;
	RetCode ret;
	DecHandle handle = dec->handle;
	FrameBuffer *fb;
	struct frame_buf **pfbpool;
	struct vpu_display *disp = NULL;
	int stride;
	vpu_mem_desc *mvcol_md = NULL;

	/* 1 extra fb for rotation(or dering) */
	if (rot_en || dering_en) {
		extrafb++;
		dec->extrafb++;
	}

	/* 1 extra fb for deblocking */
	if (mp4dblk_en) {
		extrafb++;
		dec->extrafb++;
	}

	totalfb = fbcount + extrafb;

	fb = dec->fb = calloc(totalfb, sizeof(FrameBuffer));
	if (fb == NULL) {
		printf("Failed to allocate fb\n");
		return -1;
	}

	pfbpool = dec->pfbpool = calloc(totalfb, sizeof(struct frame_buf *));
	if (pfbpool == NULL) {
		printf("Failed to allocate pfbpool\n");
		free(fb);
		return -1;
	}
	
	if ((dst_scheme != PATH_V4L2) ||
		((dst_scheme == PATH_V4L2) && (rot_en || mp4dblk_en ||
			(platform_is_mx37() && dering_en)))) {

		for (i = 0; i < totalfb; i++) {
			pfbpool[i] = framebuf_alloc(dec->stride,
						    dec->picheight);
			if (pfbpool[i] == NULL) {
				totalfb = i;
				goto err;
			}

			fb[i].bufY = pfbpool[i]->addrY;
			fb[i].bufCb = pfbpool[i]->addrCb;
			fb[i].bufCr = pfbpool[i]->addrCr;
			if (platform_is_mx37()) {
				fb[i].bufMvCol = pfbpool[i]->mvColBuf;
			}
		}
	}
	
	if (dst_scheme == PATH_V4L2) {
		rotation.rot_en = dec->cmdl->rot_en;
		rotation.rot_angle = dec->cmdl->rot_angle;
		/* use the ipu h/w rotation instead of vpu rotaton */
		if (platform_is_mx37() && rotation.rot_en) {
			rotation.rot_en = 0;
			rotation.ipu_rot_en = 1;
		}

		disp = v4l_display_open(dec->picwidth, dec->picheight,
					fbcount, rotation, dec->stride);
		if (disp == NULL) {
			goto err;
		}

		if ((rot_en == 0) && (mp4dblk_en == 0) && (dering_en == 0)) {
			img_size = dec->stride * dec->picheight;

			if (platform_is_mx37()) {
				mvcol_md = dec->mvcol_memdesc =
					calloc(fbcount, sizeof(vpu_mem_desc));
			}
			for (i = 0; i < fbcount; i++) {
				fb[i].bufY = disp->buffers[i]->offset;
				fb[i].bufCb = fb[i].bufY + img_size;
				fb[i].bufCr = fb[i].bufCb + (img_size >> 2);
				/* allocate MvCol buffer here */
				if (platform_is_mx37()) {
					memset(&mvcol_md[i], 0,
							sizeof(vpu_mem_desc));
					mvcol_md[i].size = img_size >> 2;
					ret = IOGetPhyMem(&mvcol_md[i]);
					if (ret) {
						printf("buffer alloc failed\n");
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
		printf("Register frame buffer failed\n");
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
		((dst_scheme == PATH_V4L2) && (rot_en || mp4dblk_en ||
			(platform_is_mx37() && dering_en)))) {
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
	if (platform_is_mxc30031()) {
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
		printf("vpu_DecGetInitialInfo failed %d\n", ret);
		return -1;
	}

	printf("Decoder: width = %d, height = %d, fps = %lu, count = %u\n",
			initinfo.picWidth, initinfo.picHeight,
			initinfo.frameRateInfo,
			initinfo.minFrameBufferCount);

	dec->fbcount = initinfo.minFrameBufferCount;
	dec->picwidth = initinfo.picWidth;
	dec->picheight = initinfo.picHeight;
	if ((dec->picwidth == 0) || (dec->picheight == 0))
		return -1;

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
	oparam.mp4DeblkEnable = dec->cmdl->mp4dblk_en;
	oparam.reorderEnable = 0; // FIXME

	oparam.psSaveBuffer = dec->phy_ps_buf;
	oparam.psSaveBufferSize = PS_SAVE_SIZE;

	ret = vpu_DecOpen(&handle, &oparam);
	if (ret != RETCODE_SUCCESS) {
		printf("vpu_DecOpen failed\n");
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
		printf("Failed to allocate decode structure\n");
		return -1;
	}

	mem_desc.size = STREAM_BUF_SIZE;
	ret = IOGetPhyMem(&mem_desc);
	if (ret) {
		printf("Unable to obtain physical mem\n");
		return -1;
	}

	if (IOGetVirtMem(&mem_desc) <= 0) {
		printf("Unable to obtain virtual mem\n");
		IOFreePhyMem(&mem_desc);
		free(dec);
		return -1;
	}

	dec->phy_bsbuf_addr = mem_desc.phy_addr;
	dec->virt_bsbuf_addr = mem_desc.virt_uaddr;
	dec->cmdl = cmdl;

	if (cmdl->format == STD_AVC) {
		ps_mem_desc.size = PS_SAVE_SIZE;
		ret = IOGetPhyMem(&ps_mem_desc);
		if (ret) {
			printf("Unable to obtain physical ps save mem\n");
			goto err;
		}

		slice_mem_desc.size = SLICE_SAVE_SIZE;
		ret = IOGetPhyMem(&slice_mem_desc);
		if (ret) {
			printf("Unable to obtain physical slice save mem\n");
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
		printf("dec_fill_bsbuffer failed\n");
		goto err1;
	}

	/* parse the bitstream */
	ret = decoder_parse(dec);
	if (ret) {
		printf("decoder parse failed\n");
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

