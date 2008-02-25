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
#include <string.h>
#include <stdlib.h>
#include "vpu_test.h"


/* V4L2 capture buffers are obtained from here */
extern struct capture_testbuffer cap_buffers[];

/* When app need to exit */
extern int quitflag;

#if STREAM_ENC_PIC_RESET == 0
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
		printf("EncGetBitstreamBuffer failed\n");
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
			printf("EncUpdateBitstreamBuffer failed\n");
			return -1;
		}
	}
			
	return space;
}
#endif

static int
encoder_fill_headers(struct encode *enc)
{
	EncHeaderParam enchdr_param = {0};
	EncHandle handle = enc->handle;
	RetCode ret;

#if STREAM_ENC_PIC_RESET == 1
	u32 vbuf;
	u32 phy_bsbuf  = enc->phy_bsbuf_addr;
	u32 virt_bsbuf = enc->virt_bsbuf_addr;
#endif

	/* Must put encode header before encoding */
	if (enc->cmdl->format == STD_MPEG4) {
		enchdr_param.headerType = VOS_HEADER;
		lock(enc->cmdl);
		vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &enchdr_param);
		unlock(enc->cmdl);

#if STREAM_ENC_PIC_RESET == 1
		vbuf = (virt_bsbuf + enchdr_param.buf - phy_bsbuf);
		ret = vpu_write(enc->cmdl, (void *)vbuf, enchdr_param.size);
		if (ret < 0)
			return -1;
#endif

		enchdr_param.headerType = VIS_HEADER;
		lock(enc->cmdl);
		vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &enchdr_param);
		unlock(enc->cmdl);

#if STREAM_ENC_PIC_RESET == 1
		vbuf = (virt_bsbuf + enchdr_param.buf - phy_bsbuf);
		ret = vpu_write(enc->cmdl, (void *)vbuf, enchdr_param.size);
		if (ret < 0)
			return -1;
#endif

		enchdr_param.headerType = VOL_HEADER;
		lock(enc->cmdl);
		vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &enchdr_param);
		unlock(enc->cmdl);

#if STREAM_ENC_PIC_RESET == 1
		vbuf = (virt_bsbuf + enchdr_param.buf - phy_bsbuf);
		ret = vpu_write(enc->cmdl, (void *)vbuf, enchdr_param.size);
		if (ret < 0)
			return -1;
#endif
	} else if (enc->cmdl->format == STD_AVC) {
		enchdr_param.headerType = SPS_RBSP;
		lock(enc->cmdl);
		vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
		unlock(enc->cmdl);

#if STREAM_ENC_PIC_RESET == 1
		vbuf = (virt_bsbuf + enchdr_param.buf - phy_bsbuf);
		ret = vpu_write(enc->cmdl, (void *)vbuf, enchdr_param.size);
		if (ret < 0)
			return -1;
#endif
		enchdr_param.headerType = PPS_RBSP;
		lock(enc->cmdl);
		vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
		unlock(enc->cmdl);

#if STREAM_ENC_PIC_RESET == 1
		vbuf = (virt_bsbuf + enchdr_param.buf - phy_bsbuf);
		ret = vpu_write(enc->cmdl, (void *)vbuf, enchdr_param.size);
		if (ret < 0)
			return -1;
#endif
	}

	return 0;
}

void
encoder_free_framebuffer(struct encode *enc)
{
	int i;

	for (i = 0; i < enc->fbcount; i++) {
		framebuf_free(enc->pfbpool[i]);
	}

	free(enc->fb);
	free(enc->pfbpool);
}

int
encoder_allocate_framebuffer(struct encode *enc)
{
	EncHandle handle = enc->handle;
	int i, stride, src_fbid = enc->src_fbid, fbcount = enc->fbcount;
	RetCode ret;
	FrameBuffer *fb;
	struct frame_buf **pfbpool;

	fb = enc->fb = calloc(fbcount + 1, sizeof(FrameBuffer));
	if (fb == NULL) {
		printf("Failed to allocate enc->fb\n");
		return -1;
	}
	
	pfbpool = enc->pfbpool = calloc(fbcount + 1,
					sizeof(struct frame_buf *));
	if (pfbpool == NULL) {
		printf("Failed to allocate enc->pfbpool\n");
		free(fb);
		return -1;
	}

	for (i = 0; i < fbcount; i++) {
		pfbpool[i] = framebuf_alloc(enc->picwidth, enc->picheight);
		if (pfbpool[i] == NULL) {
			fbcount = i;
			goto err1;
		}
		
		fb[i].bufY = pfbpool[i]->addrY;
		fb[i].bufCb = pfbpool[i]->addrCb;
		fb[i].bufCr = pfbpool[i]->addrCr;
	}
	
	/* Must be a multiple of 16 */
	stride = (enc->picwidth + 15) & ~15;

	lock(enc->cmdl);
	ret = vpu_EncRegisterFrameBuffer(handle, fb, fbcount, stride);
	unlock(enc->cmdl);
	if (ret != RETCODE_SUCCESS) {
		printf("Register frame buffer failed\n");
		goto err1;
	}
	
	if (enc->cmdl->src_scheme == PATH_V4L2) {
		ret = v4l_capture_setup(enc->picwidth, enc->picheight, 30);
		if (ret < 0) {
			goto err1;
		}
	} else {
		/* Allocate a single frame buffer for source frame */
		pfbpool[src_fbid] = framebuf_alloc(enc->picwidth,
						   enc->picheight);
		if (pfbpool[src_fbid] == NULL) {
			printf("failed to allocate single framebuf\n");
			goto err1;
		}
		
		fb[src_fbid].bufY = pfbpool[src_fbid]->addrY;
		fb[src_fbid].bufCb = pfbpool[src_fbid]->addrCb;
		fb[src_fbid].bufCr = pfbpool[src_fbid]->addrCr;
		enc->fbcount++;
	}

	return 0;

err1:
	for (i = 0; i < fbcount; i++) {
		framebuf_free(pfbpool[i]);
	}

	free(fb);
	free(pfbpool);
	return -1;
}

static int
encoder_start(struct encode *enc)
{
	EncHandle handle = enc->handle;
	EncParam  enc_param = {0};
	EncOutputInfo outinfo = {0};
	RetCode ret = 0;
	int src_fbid = enc->src_fbid, img_size, frame_id = 0;
	FrameBuffer *fb = enc->fb;
	struct frame_buf **pfbpool = enc->pfbpool;
	struct frame_buf *pfb;
	u32 yuv_addr;
	struct v4l2_buffer v4l2_buf;
	int src_fd = enc->cmdl->src_fd;
	int src_scheme = enc->cmdl->src_scheme;
	int count = enc->cmdl->count;

#if STREAM_ENC_PIC_RESET == 0
	PhysicalAddress phy_bsbuf_start = enc->phy_bsbuf_addr;
	u32 virt_bsbuf_start = enc->virt_bsbuf_addr;
	u32 virt_bsbuf_end = virt_bsbuf_start + STREAM_BUF_SIZE;
#else
	u32 vbuf;
#endif

	/* Must put encode header before encoding */
	ret = encoder_fill_headers(enc);
	if (ret) {
		printf("Encode fill headers failed\n");
		return -1;
	}

	enc_param.sourceFrame = &enc->fb[src_fbid];
	enc_param.quantParam = 30;
	enc_param.forceIPicture = 0;
	enc_param.skipPicture = 0;

	if (src_scheme == PATH_V4L2) {
		ret = v4l_start_capturing();
		if (ret < 0) {
			return -1;
		}

		img_size = enc->picwidth * enc->picheight;
	} else {
		img_size = enc->picwidth * enc->picheight * 3 / 2;
	}

	/* The main encoding loop */
	while (1) {
		if (src_scheme == PATH_V4L2) {
			ret = v4l_get_capture_data(&v4l2_buf);
			if (ret < 0) {
				goto err2;
			}
			
			fb[src_fbid].bufY = cap_buffers[v4l2_buf.index].offset;
			fb[src_fbid].bufCb = fb[src_fbid].bufY + img_size;
			fb[src_fbid].bufCr = fb[src_fbid].bufCb +
						(img_size >> 2);
		} else {
			pfb = pfbpool[src_fbid];
			yuv_addr = pfb->addrY + pfb->desc.virt_uaddr -
					pfb->desc.phy_addr;
			ret = freadn(src_fd, (void *)yuv_addr, img_size);
			if (ret <= 0)
				break;
		}

		if ((frame_id % 10) == 0) {
			enc_param.forceIPicture = 1;
			enc->cmdl->iframe = 1;
		} else {
			enc_param.forceIPicture = 0;
			enc->cmdl->iframe = 0;
		}

		lock(enc->cmdl);
		ret = vpu_EncStartOneFrame(handle, &enc_param);
		if (ret != RETCODE_SUCCESS) {
			printf("EncStartOneFrame failed\n");
			unlock(enc->cmdl);
			goto err2;
		}
		
		while (vpu_IsBusy()) {
#if STREAM_ENC_PIC_RESET == 0
			ret = enc_readbs_ring_buffer(handle, enc->cmdl,
					virt_bsbuf_start, virt_bsbuf_end,
					phy_bsbuf_start, STREAM_READ_SIZE);
			if (ret < 0) {
				unlock(enc->cmdl);
				goto err2;
			}
#endif
			vpu_WaitForInt(200);
		}

		ret = vpu_EncGetOutputInfo(handle, &outinfo);
		unlock(enc->cmdl);
		if (ret != RETCODE_SUCCESS) {
			printf("EncGetOutputInfo failed\n");
			goto err2;
		}

		if (src_scheme == PATH_V4L2) {
			v4l_put_capture_data(&v4l2_buf);
		}

		if (quitflag)
			break;

#if STREAM_ENC_PIC_RESET == 1
		vbuf = (enc->virt_bsbuf_addr + outinfo.bitstreamBuffer
					- enc->phy_bsbuf_addr);
		ret = vpu_write(enc->cmdl, (void *)vbuf, outinfo.bitstreamSize);
		if (ret < 0) {
			printf("writing bitstream buffer failed\n");
			goto err2;
		}
#endif
		frame_id++;
		if ((count != 0) && (frame_id >= count))
			break;
	}

#if STREAM_ENC_PIC_RESET == 0
	enc_readbs_ring_buffer(handle, enc->cmdl, virt_bsbuf_start,
			virt_bsbuf_end, phy_bsbuf_start, 0);
#endif

	printf("Finished encoding: %d frames\n", frame_id);
err2:
	if (src_scheme == PATH_V4L2) {
		v4l_stop_capturing();
	}

	/* Inform the other end that no more frames will be sent */
	if (enc->cmdl->dst_scheme == PATH_NET) {
		vpu_write(enc->cmdl, NULL, 0);
	}

	/* For automation of test case */
	if (ret > 0)
		ret = 0;
	
	return ret;
}

int
encoder_configure(struct encode *enc)
{
	EncHandle handle = enc->handle;
	SearchRamParam search_pa = {0};
	EncInitialInfo initinfo = {0};
	RetCode ret;
	MirrorDirection mirror;

	if (platform_is_mx27()) {
		search_pa.searchRamAddr = 0xFFFF4C00;
	} else if (platform_is_mxc30031()) {
		search_pa.searchRamAddr = 0xD0000000;
		search_pa.SearchRamSize =
			((enc->picwidth + 15) & ~15) * 36 + 2048;
	}

	lock(enc->cmdl);
	ret = vpu_EncGiveCommand(handle, ENC_SET_SEARCHRAM_PARAM, &search_pa);
	if (ret != RETCODE_SUCCESS) {
		printf("Encoder SET_SEARCHRAM_PARAM failed\n");
		unlock(enc->cmdl);
		return -1;
	}

	if (enc->cmdl->rot_en) {
		vpu_EncGiveCommand(handle, ENABLE_ROTATION, 0);
		vpu_EncGiveCommand(handle, ENABLE_MIRRORING, 0);
		vpu_EncGiveCommand(handle, SET_ROTATION_ANGLE,
					&enc->cmdl->rot_angle);
		mirror = enc->cmdl->mirror;
		vpu_EncGiveCommand(handle, SET_MIRROR_DIRECTION, &mirror);
	}

	ret = vpu_EncGetInitialInfo(handle, &initinfo);
	unlock(enc->cmdl);
	if (ret != RETCODE_SUCCESS) {
		printf("Encoder GetInitialInfo failed\n");
		return -1;
	}

	enc->fbcount = enc->src_fbid = initinfo.minFrameBufferCount;

	if (enc->cmdl->save_enc_hdr) {
		if (enc->cmdl->format == STD_MPEG4) {
			SaveGetEncodeHeader(handle, ENC_GET_VOS_HEADER,
						"mp4_vos_header.dat");
			SaveGetEncodeHeader(handle, ENC_GET_VO_HEADER,
						"mp4_vo_header.dat");
			SaveGetEncodeHeader(handle, ENC_GET_VOL_HEADER,
						"mp4_vol_header.dat");
		} else if (enc->cmdl->format == STD_AVC) {
			SaveGetEncodeHeader(handle, ENC_GET_SPS_RBSP,
						"avc_sps_header.dat");
			SaveGetEncodeHeader(handle, ENC_GET_PPS_RBSP,
						"avc_pps_header.dat");
		}
	}

	return 0;
}

void
encoder_close(struct encode *enc)
{
	EncOutputInfo outinfo = {0};
	RetCode ret;

	lock(enc->cmdl);
	ret = vpu_EncClose(enc->handle);
	if (ret == RETCODE_FRAME_NOT_COMPLETE) {
		vpu_EncGetOutputInfo(enc->handle, &outinfo);
		vpu_EncClose(enc->handle);
	}
	unlock(enc->cmdl);
}

int
encoder_open(struct encode *enc)
{
	EncHandle handle = {0};
	EncOpenParam encop = {0};
	RetCode ret;

	/* Fill up parameters for encoding */
	encop.bitstreamBuffer = enc->phy_bsbuf_addr;
	encop.bitstreamBufferSize = STREAM_BUF_SIZE;
	encop.bitstreamFormat = enc->cmdl->format;

	if (enc->cmdl->width && enc->cmdl->height) {
		enc->picwidth = enc->cmdl->width;
		enc->picheight = enc->cmdl->height;
	}
	
	/* If rotation angle is 90 or 270, pic width and height are swapped */
	if (enc->cmdl->rot_angle == 90 || enc->cmdl->rot_angle == 270) {
		encop.picWidth = enc->picheight;
		encop.picHeight = enc->picwidth;
	} else {
		encop.picWidth = enc->picwidth;
		encop.picHeight = enc->picheight;
	}

	encop.frameRateInfo = 30;
	encop.bitRate = enc->cmdl->bitrate;
	encop.gopSize = enc->cmdl->gop;
	encop.slicemode.sliceMode = 1;
	encop.slicemode.sliceSizeMode = 0;
	encop.slicemode.sliceSize = 4000;

	encop.initialDelay = 0;
	encop.vbvBufferSize = 0;        /* 0 = ignore 8 */
	encop.enableAutoSkip = 1;
	encop.intraRefresh = 0;
	encop.sliceReport = 0;
	encop.mbReport = 0;
	encop.mbQpReport = 0;
	encop.rcIntraQp = -1;
	encop.ringBufferEnable = 0;
	encop.dynamicAllocEnable = 0;

	if (enc->cmdl->format == STD_MPEG4) {
		encop.EncStdParam.mp4Param.mp4_dataPartitionEnable = 0;
		encop.EncStdParam.mp4Param.mp4_reversibleVlcEnable = 0;
		encop.EncStdParam.mp4Param.mp4_intraDcVlcThr = 0;
		encop.EncStdParam.mp4Param.mp4_hecEnable = 0;
		encop.EncStdParam.mp4Param.mp4_verid = 2;
	} else if ( enc->cmdl->format == STD_H263) {
		encop.EncStdParam.h263Param.h263_annexJEnable = 0;
		encop.EncStdParam.h263Param.h263_annexKEnable = 0;
		encop.EncStdParam.h263Param.h263_annexTEnable = 0;
	} else if (enc->cmdl->format == STD_AVC) {
		encop.EncStdParam.avcParam.avc_constrainedIntraPredFlag = 0;
		encop.EncStdParam.avcParam.avc_disableDeblk = 0;
		encop.EncStdParam.avcParam.avc_deblkFilterOffsetAlpha = 0;
		encop.EncStdParam.avcParam.avc_deblkFilterOffsetBeta = 0;
		encop.EncStdParam.avcParam.avc_chromaQpOffset = 0;
		encop.EncStdParam.avcParam.avc_audEnable = 0;
		encop.EncStdParam.avcParam.avc_fmoEnable = 0;
		encop.EncStdParam.avcParam.avc_fmoType = 0;
		encop.EncStdParam.avcParam.avc_fmoSliceNum = 0;
	}

	ret = vpu_EncOpen(&handle, &encop);
	if (ret != RETCODE_SUCCESS) {
		printf("Encoder open failed %d\n", ret);
		return -1;
	}

	enc->handle = handle;
	return 0;
}

int
encode_test(void *arg)
{
	struct cmd_line *cmdl = (struct cmd_line *)arg;
	vpu_mem_desc	mem_desc = {0};
	struct encode *enc;
	int ret = 0;

	/* sleep some time so that we have time to start the server */
	if (cmdl->dst_scheme == PATH_NET) {
		sleep(10);
	}

	/* allocate memory for must remember stuff */
	enc = (struct encode *)calloc(1, sizeof(struct encode));
	if (enc == NULL) {
		printf("Failed to allocate encode structure\n");
		return -1;
	}

	/* get physical contigous bit stream buffer */
	mem_desc.size = STREAM_BUF_SIZE;
	ret = IOGetPhyMem(&mem_desc);
	if (ret) {
		printf("Unable to obtain physical memory\n");
		free(enc);
		return -1;
	}

	/* mmap that physical buffer */
	enc->virt_bsbuf_addr = IOGetVirtMem(&mem_desc);
	if (enc->virt_bsbuf_addr <= 0) {
		printf("Unable to map physical memory\n");
		IOFreePhyMem(&mem_desc);
		free(enc);
		return -1;
	}

	enc->phy_bsbuf_addr = mem_desc.phy_addr;
	enc->cmdl = cmdl;

	/* open the encoder */
	ret = encoder_open(enc);
	if (ret)
		goto err;

	/* configure the encoder */
	ret = encoder_configure(enc);
	if (ret)
		goto err1;

	/* allocate memory for the frame buffers */
	ret = encoder_allocate_framebuffer(enc);
	if (ret)
		goto err1;

	/* start encoding */
	ret = encoder_start(enc);
	
	/* free the allocated framebuffers */
	encoder_free_framebuffer(enc);
err1:
	/* close the encoder */
	encoder_close(enc);
err:
	/* free the physical memory */
	IOFreeVirtMem(&mem_desc);
	IOFreePhyMem(&mem_desc);
	free(enc);
	return ret;
}

