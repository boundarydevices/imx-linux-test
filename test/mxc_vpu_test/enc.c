/*
 * Copyright 2004-2012 Freescale Semiconductor, Inc.
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
#include "vpu_jpegtable.h"


/* V4L2 capture buffers are obtained from here */
extern struct capture_testbuffer cap_buffers[];

/* When app need to exit */
extern int quitflag;
static int frameRateInfo = 0;

#define FN_ENC_QP_DATA "enc_qp.log"
#define FN_ENC_SLICE_BND_DATA "enc_slice_bnd.log"
#define FN_ENC_MV_DATA "enc_mv.log"
#define FN_ENC_SLICE_DATA "enc_slice.log"
static FILE *fpEncSliceBndInfo = NULL;
static FILE *fpEncQpInfo = NULL;
static FILE *fpEncMvInfo = NULL;
static FILE *fpEncSliceInfo = NULL;

void SaveEncMbInfo(u8 *mbParaBuf, int size, int MbNumX, int EncNum)
{
	int i;
	if(!fpEncQpInfo)
		fpEncQpInfo = fopen(FN_ENC_QP_DATA, "w+");
	if(!fpEncQpInfo)
		return;

	if(!fpEncSliceBndInfo)
		fpEncSliceBndInfo = fopen(FN_ENC_SLICE_BND_DATA, "w+");
	if(!fpEncSliceBndInfo)
		return;

	fprintf(fpEncQpInfo, "FRAME [%1d]\n", EncNum);
	fprintf(fpEncSliceBndInfo, "FRAME [%1d]\n", EncNum);

	for(i=0; i < size; i++) {
		fprintf(fpEncQpInfo, "MbAddr[%4d]: MbQs[%2d]\n", i, mbParaBuf[i] & 63);
		fprintf(fpEncSliceBndInfo, "MbAddr[%4d]: Slice Boundary Flag[%1d]\n",
			 i, (mbParaBuf[i] >> 6) & 1);
	}

	fprintf(fpEncQpInfo, "\n");
	fprintf(fpEncSliceBndInfo, "\n");
	fflush(fpEncQpInfo);
	fflush(fpEncSliceBndInfo);
}

void SaveEncMvInfo(u8 *mvParaBuf, int size, int MbNumX, int EncNum)
{
	int i;
	if(!fpEncMvInfo)
		fpEncMvInfo = fopen(FN_ENC_MV_DATA, "w+");
	if(!fpEncMvInfo)
		return;

	fprintf(fpEncMvInfo, "FRAME [%1d]\n", EncNum);
	for(i=0; i<size/4; i++) {
		u16 mvX = (mvParaBuf[0] << 8) | (mvParaBuf[1]);
		u16 mvY = (mvParaBuf[2] << 8) | (mvParaBuf[3]);
		if(mvX & 0x8000) {
			fprintf(fpEncMvInfo, "MbAddr[%4d:For ]: Avail[0] Mv[%5d:%5d]\n", i, 0, 0);
		} else {
			mvX = (mvX & 0x7FFF) | ((mvX << 1) & 0x8000);
			fprintf(fpEncMvInfo, "MbAddr[%4d:For ]: Avail[1] Mv[%5d:%5d]\n", i, mvX, mvY);
		}
		mvParaBuf += 4;
	}
	fprintf(fpEncMvInfo, "\n");
	fflush(fpEncMvInfo);
}

void SaveEncSliceInfo(u8 *SliceParaBuf, int size, int EncNum)
{
	int i, nMbAddr, nSliceBits;
	if(!fpEncSliceInfo)
		fpEncSliceInfo = fopen(FN_ENC_SLICE_DATA, "w+");
	if(!fpEncSliceInfo)
		return;

	fprintf(fpEncSliceInfo, "EncFrmNum[%3d]\n", EncNum);

	for(i=0; i<size / 8; i++) {
		nMbAddr = (SliceParaBuf[2] << 8) | SliceParaBuf[3];
		nSliceBits = (int)(SliceParaBuf[4] << 24)|(SliceParaBuf[5] << 16)|
				(SliceParaBuf[6] << 8)|(SliceParaBuf[7]);
		fprintf(fpEncSliceInfo, "[%2d] mbIndex.%3d, Bits.%d\n", i, nMbAddr, nSliceBits);
		SliceParaBuf += 8;
	}

	fprintf(fpEncSliceInfo, "\n");
	fflush(fpEncSliceInfo);
}

void jpgGetHuffTable(EncMjpgParam *param)
{
	/* Rearrange and insert pre-defined Huffman table to deticated variable. */
	memcpy(param->huffBits[DC_TABLE_INDEX0], lumaDcBits, 16);   /* Luma DC BitLength */
	memcpy(param->huffVal[DC_TABLE_INDEX0], lumaDcValue, 16);   /* Luma DC HuffValue */

	memcpy(param->huffBits[AC_TABLE_INDEX0], lumaAcBits, 16);   /* Luma DC BitLength */
	memcpy(param->huffVal[AC_TABLE_INDEX0], lumaAcValue, 162);  /* Luma DC HuffValue */

	memcpy(param->huffBits[DC_TABLE_INDEX1], chromaDcBits, 16); /* Chroma DC BitLength */
	memcpy(param->huffVal[DC_TABLE_INDEX1], chromaDcValue, 16); /* Chroma DC HuffValue */

	memcpy(param->huffBits[AC_TABLE_INDEX1], chromaAcBits, 16); /* Chroma AC BitLength */
	memcpy(param->huffVal[AC_TABLE_INDEX1], chromaAcValue, 162); /* Chorma AC HuffValue */
}

void jpgGetQMatrix(EncMjpgParam *param)
{
	/* Rearrange and insert pre-defined Q-matrix to deticated variable. */
	memcpy(param->qMatTab[DC_TABLE_INDEX0], lumaQ2, 64);
	memcpy(param->qMatTab[AC_TABLE_INDEX0], chromaBQ2, 64);

	memcpy(param->qMatTab[DC_TABLE_INDEX1], param->qMatTab[DC_TABLE_INDEX0], 64);
	memcpy(param->qMatTab[AC_TABLE_INDEX1], param->qMatTab[AC_TABLE_INDEX0], 64);
}

void jpgGetCInfoTable(EncMjpgParam *param)
{
	int format = param->mjpg_sourceFormat;

	memcpy(param->cInfoTab, cInfoTable[format], 6 * 4);
}

static int
enc_readbs_reset_buffer(struct encode *enc, PhysicalAddress paBsBufAddr, int bsBufsize)
{
	u32 vbuf;

	vbuf = enc->virt_bsbuf_addr + paBsBufAddr - enc->phy_bsbuf_addr;
	return vpu_write(enc->cmdl, (void *)vbuf, bsBufsize);
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
					(Uint32 *)&size);
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
		if (!enc->mvc_extension || !enc->mvc_paraset_refresh_en) {
			enchdr_param.headerType = SPS_RBSP;
			vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
			if (enc->ringBufferEnable == 0 ) {
				ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
				if (ret < 0)
					return -1;
			}
		}

		if (enc->mvc_extension) {
			enchdr_param.headerType = SPS_RBSP_MVC;
			vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
			if (enc->ringBufferEnable == 0 ) {
				ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
				if (ret < 0)
					return -1;
			}
		}

		enchdr_param.headerType = PPS_RBSP;
		vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
		if (enc->ringBufferEnable == 0 ) {
			ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
			if (ret < 0)
				return -1;
		}

		if (enc->mvc_extension) { /* MVC */
			enchdr_param.headerType = PPS_RBSP_MVC;
			vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
			if (enc->ringBufferEnable == 0 ) {
				ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
				if (ret < 0)
					return -1;
			}
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

void
encoder_free_framebuffer(struct encode *enc)
{
	int i;

	for (i = 0; i < enc->totalfb; i++) {
		framebuf_free(enc->pfbpool[i]);
	}

	free(enc->fb);
	free(enc->pfbpool);
}

int
encoder_allocate_framebuffer(struct encode *enc)
{
	EncHandle handle = enc->handle;
	int i, enc_stride, src_stride, src_fbid;
	int totalfb, minfbcount, srcfbcount, extrafbcount;
	RetCode ret;
	FrameBuffer *fb;
	PhysicalAddress subSampBaseA = 0, subSampBaseB = 0;
	struct frame_buf **pfbpool;
	EncExtBufInfo extbufinfo = {0};
	int enc_fbwidth, enc_fbheight, src_fbwidth, src_fbheight;

	minfbcount = enc->minFrameBufferCount;
	srcfbcount = 1;

	enc_fbwidth = (enc->enc_picwidth + 15) & ~15;
	enc_fbheight = (enc->enc_picheight + 15) & ~15;
	src_fbwidth = (enc->src_picwidth + 15) & ~15;
	src_fbheight = (enc->src_picheight + 15) & ~15;

	if (cpu_is_mx6x()) {
		if (enc->cmdl->format == STD_AVC && enc->mvc_extension) /* MVC */
			extrafbcount = 2 + 2; /* Subsamp [2] + Subsamp MVC [2] */
		else if (enc->cmdl->format == STD_MJPG)
			extrafbcount = 0;
		else
			extrafbcount = 2; /* Subsamp buffer [2] */
	} else
		extrafbcount = 0;

	enc->totalfb = totalfb = minfbcount + extrafbcount + srcfbcount;

	/* last framebuffer is used as src frame in the test */
	enc->src_fbid = src_fbid = totalfb - 1;

	fb = enc->fb = calloc(totalfb, sizeof(FrameBuffer));
	if (fb == NULL) {
		err_msg("Failed to allocate enc->fb\n");
		return -1;
	}

	pfbpool = enc->pfbpool = calloc(totalfb,
					sizeof(struct frame_buf *));
	if (pfbpool == NULL) {
		err_msg("Failed to allocate enc->pfbpool\n");
		free(fb);
		return -1;
	}

	if (enc->cmdl->mapType == LINEAR_FRAME_MAP) {
		/* All buffers are linear */
		for (i = 0; i < minfbcount + extrafbcount; i++) {
			pfbpool[i] = framebuf_alloc(enc->cmdl->format, enc->mjpg_fmt,
						    enc_fbwidth, enc_fbheight, 0);
			if (pfbpool[i] == NULL) {
				goto err1;
			}
		}
	 } else {
		/* Encoded buffers are tiled */
		for (i = 0; i < minfbcount; i++) {
			pfbpool[i] = tiled_framebuf_alloc(enc->cmdl->format, enc->mjpg_fmt,
					    enc_fbwidth, enc_fbheight, 0, enc->cmdl->mapType);
			if (pfbpool[i] == NULL)
				goto err1;
		}
		/* sub frames are linear */
		for (i = minfbcount; i < minfbcount + extrafbcount; i++) {
			pfbpool[i] = framebuf_alloc(enc->cmdl->format, enc->mjpg_fmt,
						    enc_fbwidth, enc_fbheight, 0);
			if (pfbpool[i] == NULL)
				goto err1;
		}
	}

	for (i = 0; i < minfbcount + extrafbcount; i++) {
		fb[i].myIndex = i;
		fb[i].bufY = pfbpool[i]->addrY;
		fb[i].bufCb = pfbpool[i]->addrCb;
		fb[i].bufCr = pfbpool[i]->addrCr;
		fb[i].strideY = pfbpool[i]->strideY;
		fb[i].strideC = pfbpool[i]->strideC;
	}

	if (cpu_is_mx6x() && (enc->cmdl->format != STD_MJPG)) {
		subSampBaseA = fb[minfbcount].bufY;
		subSampBaseB = fb[minfbcount + 1].bufY;
		if (enc->cmdl->format == STD_AVC && enc->mvc_extension) { /* MVC */
			extbufinfo.subSampBaseAMvc = fb[minfbcount + 2].bufY;
			extbufinfo.subSampBaseBMvc = fb[minfbcount + 3].bufY;
		}
	}

	/* Must be a multiple of 16 */
	if (enc->cmdl->rot_angle == 90 || enc->cmdl->rot_angle == 270)
		enc_stride = (enc->enc_picheight + 15 ) & ~15;
	else
		enc_stride = (enc->enc_picwidth + 15) & ~15;
	src_stride = (enc->src_picwidth + 15 ) & ~15;

	extbufinfo.scratchBuf = enc->scratchBuf;
	ret = vpu_EncRegisterFrameBuffer(handle, fb, minfbcount, enc_stride, src_stride,
					    subSampBaseA, subSampBaseB, &extbufinfo);
	if (ret != RETCODE_SUCCESS) {
		err_msg("Register frame buffer failed\n");
		goto err1;
	}

	if (enc->cmdl->src_scheme == PATH_V4L2) {
		ret = v4l_capture_setup(enc, enc->src_picwidth, enc->src_picheight, 30);
		if (ret < 0) {
			goto err1;
		}
	} else {
		/* Allocate a single frame buffer for source frame */
		pfbpool[src_fbid] = framebuf_alloc(enc->cmdl->format, enc->mjpg_fmt,
						   src_fbwidth, src_fbheight, 0);
		if (pfbpool[src_fbid] == NULL) {
			err_msg("failed to allocate single framebuf\n");
			goto err1;
		}

		fb[src_fbid].myIndex = enc->src_fbid;
		fb[src_fbid].bufY = pfbpool[src_fbid]->addrY;
		fb[src_fbid].bufCb = pfbpool[src_fbid]->addrCb;
		fb[src_fbid].bufCr = pfbpool[src_fbid]->addrCr;
		fb[src_fbid].strideY = pfbpool[src_fbid]->strideY;
		fb[src_fbid].strideC = pfbpool[src_fbid]->strideC;
	}

	return 0;

err1:
	for (i = 0; i < totalfb; i++) {
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
	EncOpenParam encop = {0};
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
	struct timeval tenc_begin,tenc_end, total_start, total_end;
	int sec, usec, loop_id;
	float tenc_time = 0, total_time=0;
	PhysicalAddress phy_bsbuf_start = enc->phy_bsbuf_addr;
	u32 virt_bsbuf_start = enc->virt_bsbuf_addr;
	u32 virt_bsbuf_end = virt_bsbuf_start + STREAM_BUF_SIZE;

	/* Must put encode header here before encoding for all codec, except MX6 MJPG */
	if (!(cpu_is_mx6x() && (enc->cmdl->format == STD_MJPG))) {
		ret = encoder_fill_headers(enc);
		if (ret) {
			err_msg("Encode fill headers failed\n");
			return -1;
		}
	}

	enc_param.sourceFrame = &enc->fb[src_fbid];
	enc_param.quantParam = 23;
	enc_param.forceIPicture = 0;
	enc_param.skipPicture = 0;
	enc_param.enableAutoSkip = 1;

	enc_param.encLeftOffset = 0;
	enc_param.encTopOffset = 0;
	if ((enc_param.encLeftOffset + enc->enc_picwidth) > enc->src_picwidth) {
		err_msg("Configure is failure for width and left offset\n");
		return -1;
	}
	if ((enc_param.encTopOffset + enc->enc_picheight) > enc->src_picheight) {
		err_msg("Configure is failure for height and top offset\n");
		return -1;
	}

	/* Set report info flag */
	if (enc->mbInfo.enable) {
		ret = vpu_EncGiveCommand(handle, ENC_SET_REPORT_MBINFO, &enc->mbInfo);
		if (ret != RETCODE_SUCCESS) {
			err_msg("Failed to set MbInfo report, ret %d\n", ret);
			return -1;
		}
	}
	if (enc->mvInfo.enable) {
		ret = vpu_EncGiveCommand(handle, ENC_SET_REPORT_MVINFO, &enc->mvInfo);
		if (ret != RETCODE_SUCCESS) {
			err_msg("Failed to set MvInfo report, ret %d\n", ret);
			return -1;
		}
	}
	if (enc->sliceInfo.enable) {
		ret = vpu_EncGiveCommand(handle, ENC_SET_REPORT_SLICEINFO, &enc->sliceInfo);
		if (ret != RETCODE_SUCCESS) {
			err_msg("Failed to set slice info report, ret %d\n", ret);
			return -1;
		}
	}
	if (src_scheme == PATH_V4L2) {
		ret = v4l_start_capturing();
		if (ret < 0) {
			return -1;
		}

		img_size = enc->src_picwidth * enc->src_picheight;
	} else {
		img_size = enc->src_picwidth * enc->src_picheight * 3 / 2;
		if (enc->cmdl->format == STD_MJPG) {
			if (enc->mjpg_fmt == MODE422 || enc->mjpg_fmt == MODE224)
				img_size = enc->src_picwidth * enc->src_picheight * 2;
			else if (enc->mjpg_fmt == MODE400)
				img_size = enc->src_picwidth * enc->src_picheight;
		}
	}

	gettimeofday(&total_start, NULL);

	/* The main encoding loop */
	while (1) {
		if (src_scheme == PATH_V4L2) {
			ret = v4l_get_capture_data(&v4l2_buf);
			if (ret < 0) {
				goto err2;
			}

			fb[src_fbid].myIndex = enc->src_fbid + v4l2_buf.index;
			fb[src_fbid].bufY = cap_buffers[v4l2_buf.index].offset;
			fb[src_fbid].bufCb = fb[src_fbid].bufY + img_size;
			if ((enc->cmdl->format == STD_MJPG) &&
			    (enc->mjpg_fmt == MODE422 || enc->mjpg_fmt == MODE224))
				fb[src_fbid].bufCr = fb[src_fbid].bufCb + (img_size >> 1);
			else
				fb[src_fbid].bufCr = fb[src_fbid].bufCb + (img_size >> 2);
			fb[src_fbid].strideY = enc->src_picwidth;
			fb[src_fbid].strideC = enc->src_picwidth / 2;
		} else {
			pfb = pfbpool[src_fbid];
			yuv_addr = pfb->addrY + pfb->desc.virt_uaddr -
					pfb->desc.phy_addr;
			ret = freadn(src_fd, (void *)yuv_addr, img_size);
			if (ret <= 0)
				break;
		}

		/* Must put encode header before each frame encoding for mx6 MJPG */
		if (cpu_is_mx6x() && (enc->cmdl->format == STD_MJPG)) {
			ret = encoder_fill_headers(enc);
			if (ret) {
				err_msg("Encode fill headers failed\n");
				goto err2;
			}
		}

		gettimeofday(&tenc_begin, NULL);
		ret = vpu_EncStartOneFrame(handle, &enc_param);
		if (ret != RETCODE_SUCCESS) {
			err_msg("vpu_EncStartOneFrame failed Err code:%d\n",
									ret);
			goto err2;
		}

		loop_id = 0;
		while (vpu_IsBusy()) {
			vpu_WaitForInt(200);
			if (enc->ringBufferEnable == 1) {
				ret = enc_readbs_ring_buffer(handle, enc->cmdl,
					virt_bsbuf_start, virt_bsbuf_end,
					phy_bsbuf_start, STREAM_READ_SIZE);
				if (ret < 0) {
					goto err2;
				}
			}
			if (loop_id == 20) {
				ret = vpu_SWReset(handle, 0);
				return -1;
			}
			loop_id ++;
		}

		gettimeofday(&tenc_end, NULL);
		sec = tenc_end.tv_sec - tenc_begin.tv_sec;
		usec = tenc_end.tv_usec - tenc_begin.tv_usec;

		if (usec < 0) {
			sec--;
			usec = usec + 1000000;
		}

		tenc_time += (sec * 1000000) + usec;

		ret = vpu_EncGetOutputInfo(handle, &outinfo);
		if (ret != RETCODE_SUCCESS) {
			err_msg("vpu_EncGetOutputInfo failed Err code: %d\n",
									ret);
			goto err2;
		}

		if (outinfo.skipEncoded)
			info_msg("Skip encoding one Frame!\n");

		if (outinfo.mbInfo.enable && outinfo.mbInfo.size && outinfo.mbInfo.addr) {
			SaveEncMbInfo(outinfo.mbInfo.addr, outinfo.mbInfo.size,
					 encop.picWidth/16, frame_id);
		}

		if (outinfo.mvInfo.enable && outinfo.mvInfo.size && outinfo.mvInfo.addr) {
			SaveEncMvInfo(outinfo.mvInfo.addr, outinfo.mvInfo.size,
					 encop.picWidth/16, frame_id);
		}

		if (outinfo.sliceInfo.enable && outinfo.sliceInfo.size &&
		    outinfo.sliceInfo.addr) {
			SaveEncSliceInfo(outinfo.sliceInfo.addr,
					     outinfo.sliceInfo.size, frame_id);
		}

		if (src_scheme == PATH_V4L2) {
			v4l_put_capture_data(&v4l2_buf);
		}

		if (quitflag)
			break;

		if (enc->ringBufferEnable == 0) {
			ret = enc_readbs_reset_buffer(enc, outinfo.bitstreamBuffer, outinfo.bitstreamSize);
			if (ret < 0) {
				err_msg("writing bitstream buffer failed\n");
				goto err2;
			}
		} else
			enc_readbs_ring_buffer(handle, enc->cmdl, virt_bsbuf_start,
						virt_bsbuf_end, phy_bsbuf_start, 0);

		frame_id++;
		if ((count != 0) && (frame_id >= count))
			break;
	}

	gettimeofday(&total_end, NULL);
	sec = total_end.tv_sec - total_start.tv_sec;
	usec = total_end.tv_usec - total_start.tv_usec;
	if (usec < 0) {
		sec--;
		usec = usec + 1000000;
	}
	total_time = (sec * 1000000) + usec;

	info_msg("Finished encoding: %d frames\n", frame_id);
	info_msg("enc fps = %.2f\n", (frame_id / (tenc_time / 1000000)));
	info_msg("total fps= %.2f \n",(frame_id / (total_time / 1000000)));

err2:
	if (src_scheme == PATH_V4L2) {
		v4l_stop_capturing();
	}

	/* Inform the other end that no more frames will be sent */
	if (enc->cmdl->dst_scheme == PATH_NET) {
		vpu_write(enc->cmdl, NULL, 0);
	}

	if (enc->mbInfo.addr)
		free(enc->mbInfo.addr);
	if (enc->mbInfo.addr)
		free(enc->mbInfo.addr);
	if (enc->sliceInfo.addr)
		free(enc->sliceInfo.addr);

	if (fpEncQpInfo) {
		fclose(fpEncQpInfo);
		fpEncQpInfo = NULL;
	}
	if (fpEncSliceBndInfo) {
		fclose(fpEncSliceBndInfo);
		fpEncSliceBndInfo = NULL;
	}
	if (fpEncMvInfo) {
		fclose(fpEncMvInfo);
		fpEncMvInfo = NULL;
	}
	if (fpEncSliceInfo) {
		fclose(fpEncSliceInfo);
		fpEncSliceInfo = NULL;
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

	if (cpu_is_mx27()) {
		search_pa.searchRamAddr = 0xFFFF4C00;
		ret = vpu_EncGiveCommand(handle, ENC_SET_SEARCHRAM_PARAM, &search_pa);
		if (ret != RETCODE_SUCCESS) {
			err_msg("Encoder SET_SEARCHRAM_PARAM failed\n");
			return -1;
		}
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
	if (ret != RETCODE_SUCCESS) {
		err_msg("Encoder GetInitialInfo failed\n");
		return -1;
	}

	enc->minFrameBufferCount = initinfo.minFrameBufferCount;
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

	enc->mbInfo.enable = 0;
	enc->mvInfo.enable = 0;
	enc->sliceInfo.enable = 0;

	if (enc->mbInfo.enable) {
		enc->mbInfo.addr = malloc(initinfo.reportBufSize.mbInfoBufSize);
		if (!enc->mbInfo.addr)
			err_msg("malloc_error\n");
	}
	if (enc->mvInfo.enable) {
		enc->mvInfo.addr = malloc(initinfo.reportBufSize.mvInfoBufSize);
		if (!enc->mvInfo.addr)
			err_msg("malloc_error\n");
	}
	if (enc->sliceInfo.enable) {
		enc->sliceInfo.addr = malloc(initinfo.reportBufSize.sliceInfoBufSize);
		if (!enc->sliceInfo.addr)
			err_msg("malloc_error\n");
	}

	return 0;
}

void
encoder_close(struct encode *enc)
{
	EncOutputInfo outinfo = {0};
	RetCode ret;

	ret = vpu_EncClose(enc->handle);
	if (ret == RETCODE_FRAME_NOT_COMPLETE) {
		vpu_SWReset(enc->handle, 0);
		vpu_EncClose(enc->handle);
	}
}

int
encoder_open(struct encode *enc)
{
	EncHandle handle = {0};
	EncOpenParam encop = {0};
	Uint8 *huffTable = enc->huffTable;
	Uint8 *qMatTable = enc->qMatTable;
	int i;
	RetCode ret;

	/* Fill up parameters for encoding */
	encop.bitstreamBuffer = enc->phy_bsbuf_addr;
	encop.bitstreamBufferSize = STREAM_BUF_SIZE;
	encop.bitstreamFormat = enc->cmdl->format;
	encop.mapType = enc->cmdl->mapType;
	encop.linear2TiledEnable = enc->linear2TiledEnable;
	/* width and height in command line means source image size */
	if (enc->cmdl->width && enc->cmdl->height) {
		enc->src_picwidth = enc->cmdl->width;
		enc->src_picheight = enc->cmdl->height;
	}

	/* enc_width and enc_height in command line means encoder output size */
	if (enc->cmdl->enc_width && enc->cmdl->enc_height) {
		enc->enc_picwidth = enc->cmdl->enc_width;
		enc->enc_picheight = enc->cmdl->enc_height;
	} else {
		enc->enc_picwidth = enc->src_picwidth;
		enc->enc_picheight = enc->src_picheight;
	}

	/* If rotation angle is 90 or 270, pic width and height are swapped */
	if (enc->cmdl->rot_angle == 90 || enc->cmdl->rot_angle == 270) {
		encop.picWidth = enc->enc_picheight;
		encop.picHeight = enc->enc_picwidth;
	} else {
		encop.picWidth = enc->enc_picwidth;
		encop.picHeight = enc->enc_picheight;
	}

	/*Note: Frame rate cannot be less than 15fps per H.263 spec */
	encop.frameRateInfo = frameRateInfo = 30;
	encop.bitRate = enc->cmdl->bitrate;
	encop.gopSize = enc->cmdl->gop;
	encop.slicemode.sliceMode = 0;	/* 0: 1 slice per picture; 1: Multiple slices per picture */
	encop.slicemode.sliceSizeMode = 0; /* 0: silceSize defined by bits; 1: sliceSize defined by MB number*/
	encop.slicemode.sliceSize = 4000;  /* Size of a slice in bits or MB numbers */

	encop.initialDelay = 0;
	encop.vbvBufferSize = 0;        /* 0 = ignore 8 */
	encop.intraRefresh = 0;
	encop.sliceReport = 0;
	encop.mbReport = 0;
	encop.mbQpReport = 0;
	encop.rcIntraQp = -1;
	encop.userQpMax = 0;
	encop.userQpMin = 0;
	encop.userQpMinEnable = 0;
	encop.userQpMaxEnable = 0;

	encop.IntraCostWeight = 0;
	encop.MEUseZeroPmv  = 0;
	/* (3: 16x16, 2:32x16, 1:64x32, 0:128x64, H.263(Short Header : always 3) */
	encop.MESearchRange = 3;

	encop.userGamma = (Uint32)(0.75*32768);         /*  (0*32768 <= gamma <= 1*32768) */
	encop.RcIntervalMode= 1;        /* 0:normal, 1:frame_level, 2:slice_level, 3: user defined Mb_level */
	encop.MbInterval = 0;
	encop.avcIntra16x16OnlyModeEnable = 0;

	encop.ringBufferEnable = enc->ringBufferEnable = 0;
	encop.dynamicAllocEnable = 0;
	encop.chromaInterleave = enc->cmdl->chromaInterleave;

	if(!cpu_is_mx6x() &&  enc->cmdl->format == STD_MJPG )
	{
		qMatTable = calloc(192,1);
		if (qMatTable == NULL) {
			err_msg("Failed to allocate qMatTable\n");
			return -1;
		}
		huffTable = calloc(432,1);
		if (huffTable == NULL) {
			free(qMatTable);
			err_msg("Failed to allocate huffTable\n");
			return -1;
		}

		/* Don't consider user defined hufftable this time */
		/* Rearrange and insert pre-defined Huffman table to deticated variable. */
		for(i = 0; i < 16; i += 4)
		{
			huffTable[i] = lumaDcBits[i + 3];
			huffTable[i + 1] = lumaDcBits[i + 2];
			huffTable[i + 2] = lumaDcBits[i + 1];
			huffTable[i + 3] = lumaDcBits[i];
		}
		for(i = 16; i < 32 ; i += 4)
		{
			huffTable[i] = lumaDcValue[i + 3 - 16];
			huffTable[i + 1] = lumaDcValue[i + 2 - 16];
			huffTable[i + 2] = lumaDcValue[i + 1 - 16];
			huffTable[i + 3] = lumaDcValue[i - 16];
		}
		for(i = 32; i < 48; i += 4)
		{
			huffTable[i] = lumaAcBits[i + 3 - 32];
			huffTable[i + 1] = lumaAcBits[i + 2 - 32];
			huffTable[i + 2] = lumaAcBits[i + 1 - 32];
			huffTable[i + 3] = lumaAcBits[i - 32];
		}
		for(i = 48; i < 216; i += 4)
		{
			huffTable[i] = lumaAcValue[i + 3 - 48];
			huffTable[i + 1] = lumaAcValue[i + 2 - 48];
			huffTable[i + 2] = lumaAcValue[i + 1 - 48];
			huffTable[i + 3] = lumaAcValue[i - 48];
		}
		for(i = 216; i < 232; i += 4)
		{
			huffTable[i] = chromaDcBits[i + 3 - 216];
			huffTable[i + 1] = chromaDcBits[i + 2 - 216];
			huffTable[i + 2] = chromaDcBits[i + 1 - 216];
			huffTable[i + 3] = chromaDcBits[i - 216];
		}
		for(i = 232; i < 248; i += 4)
		{
			huffTable[i] = chromaDcValue[i + 3 - 232];
			huffTable[i + 1] = chromaDcValue[i + 2 - 232];
			huffTable[i + 2] = chromaDcValue[i + 1 - 232];
			huffTable[i + 3] = chromaDcValue[i - 232];
		}
		for(i = 248; i < 264; i += 4)
		{
			huffTable[i] = chromaAcBits[i + 3 - 248];
			huffTable[i + 1] = chromaAcBits[i + 2 - 248];
			huffTable[i + 2] = chromaAcBits[i + 1 - 248];
			huffTable[i + 3] = chromaAcBits[i - 248];
		}
		for(i = 264; i < 432; i += 4)
		{
			huffTable[i] = chromaAcValue[i + 3 - 264];
			huffTable[i + 1] = chromaAcValue[i + 2 - 264];
			huffTable[i + 2] = chromaAcValue[i + 1 - 264];
			huffTable[i + 3] = chromaAcValue[i - 264];
		}

		/* Rearrange and insert pre-defined Q-matrix to deticated variable. */
		for(i = 0; i < 64; i += 4)
		{
			qMatTable[i] = lumaQ2[i + 3];
			qMatTable[i + 1] = lumaQ2[i + 2];
			qMatTable[i + 2] = lumaQ2[i + 1];
			qMatTable[i + 3] = lumaQ2[i];
		}
		for(i = 64; i < 128; i += 4)
		{
			qMatTable[i] = chromaBQ2[i + 3 - 64];
			qMatTable[i + 1] = chromaBQ2[i + 2 - 64];
			qMatTable[i + 2] = chromaBQ2[i + 1 - 64];
			qMatTable[i + 3] = chromaBQ2[i - 64];
		}
		for(i = 128; i < 192; i += 4)
		{
			qMatTable[i] = chromaRQ2[i + 3 - 128];
			qMatTable[i + 1] = chromaRQ2[i + 2 - 128];
			qMatTable[i + 2] = chromaRQ2[i + 1 - 128];
			qMatTable[i + 3] = chromaRQ2[i - 128];
		}
	}

	if (enc->cmdl->format == STD_MPEG4) {
		encop.EncStdParam.mp4Param.mp4_dataPartitionEnable = 0;
		enc->mp4_dataPartitionEnable =
			encop.EncStdParam.mp4Param.mp4_dataPartitionEnable;
		encop.EncStdParam.mp4Param.mp4_reversibleVlcEnable = 0;
		encop.EncStdParam.mp4Param.mp4_intraDcVlcThr = 0;
		encop.EncStdParam.mp4Param.mp4_hecEnable = 0;
		encop.EncStdParam.mp4Param.mp4_verid = 2;
	} else if ( enc->cmdl->format == STD_H263) {
		encop.EncStdParam.h263Param.h263_annexIEnable = 0;
		encop.EncStdParam.h263Param.h263_annexJEnable = 1;
		encop.EncStdParam.h263Param.h263_annexKEnable = 0;
		encop.EncStdParam.h263Param.h263_annexTEnable = 0;
	} else if (enc->cmdl->format == STD_AVC) {
		encop.EncStdParam.avcParam.avc_constrainedIntraPredFlag = 0;
		encop.EncStdParam.avcParam.avc_disableDeblk = 1;
		encop.EncStdParam.avcParam.avc_deblkFilterOffsetAlpha = 6;
		encop.EncStdParam.avcParam.avc_deblkFilterOffsetBeta = 0;
		encop.EncStdParam.avcParam.avc_chromaQpOffset = 10;
		encop.EncStdParam.avcParam.avc_audEnable = 0;
		if (cpu_is_mx6x()) {
			encop.EncStdParam.avcParam.interview_en = 0;
			encop.EncStdParam.avcParam.paraset_refresh_en = enc->mvc_paraset_refresh_en = 0;
			encop.EncStdParam.avcParam.prefix_nal_en = 0;
			encop.EncStdParam.avcParam.mvc_extension = enc->cmdl->mp4_h264Class;
			enc->mvc_extension = enc->cmdl->mp4_h264Class;
			encop.EncStdParam.avcParam.avc_frameCroppingFlag = 0;
			encop.EncStdParam.avcParam.avc_frameCropLeft = 0;
			encop.EncStdParam.avcParam.avc_frameCropRight = 0;
			encop.EncStdParam.avcParam.avc_frameCropTop = 0;
			encop.EncStdParam.avcParam.avc_frameCropBottom = 0;
			if (enc->cmdl->rot_angle != 90 &&
			    enc->cmdl->rot_angle != 270 &&
			    enc->enc_picheight == 1080) {
				/*
				 * In case of AVC encoder, when we want to use
				 * unaligned display width frameCroppingFlag
				 * parameters should be adjusted to displayable
				 * rectangle
				 */
				encop.EncStdParam.avcParam.avc_frameCroppingFlag = 1;
				encop.EncStdParam.avcParam.avc_frameCropBottom = 8;
			}

		} else {
			encop.EncStdParam.avcParam.avc_fmoEnable = 0;
			encop.EncStdParam.avcParam.avc_fmoType = 0;
			encop.EncStdParam.avcParam.avc_fmoSliceNum = 1;
			encop.EncStdParam.avcParam.avc_fmoSliceSaveBufSize = 32; /* FMO_SLICE_SAVE_BUF_SIZE */
		}
	} else if (enc->cmdl->format == STD_MJPG) {
		encop.EncStdParam.mjpgParam.mjpg_sourceFormat = enc->mjpg_fmt; /* encConfig.mjpgChromaFormat */
		encop.EncStdParam.mjpgParam.mjpg_restartInterval = 60;
		encop.EncStdParam.mjpgParam.mjpg_thumbNailEnable = 0;
		encop.EncStdParam.mjpgParam.mjpg_thumbNailWidth = 0;
		encop.EncStdParam.mjpgParam.mjpg_thumbNailHeight = 0;
		if (cpu_is_mx6x()) {
			jpgGetHuffTable(&encop.EncStdParam.mjpgParam);
			jpgGetQMatrix(&encop.EncStdParam.mjpgParam);
			jpgGetCInfoTable(&encop.EncStdParam.mjpgParam);
		} else {
			encop.EncStdParam.mjpgParam.mjpg_hufTable = huffTable;
			encop.EncStdParam.mjpgParam.mjpg_qMatTable = qMatTable;
		}
	}

	ret = vpu_EncOpen(&handle, &encop);
	if (ret != RETCODE_SUCCESS) {
		if (enc->cmdl->format == STD_MJPG) {
			free(qMatTable);
			free(huffTable);
		}
		err_msg("Encoder open failed %d\n", ret);
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
	vpu_mem_desc scratch_mem_desc = {0};
	struct encode *enc;
	int ret = 0;

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

	/* sleep some time so that we have time to start the server */
	if (cmdl->dst_scheme == PATH_NET) {
		sleep(10);
	}

	/* allocate memory for must remember stuff */
	enc = (struct encode *)calloc(1, sizeof(struct encode));
	if (enc == NULL) {
		err_msg("Failed to allocate encode structure\n");
		return -1;
	}

	/* get physical contigous bit stream buffer */
	mem_desc.size = STREAM_BUF_SIZE;
	ret = IOGetPhyMem(&mem_desc);
	if (ret) {
		err_msg("Unable to obtain physical memory\n");
		free(enc);
		return -1;
	}

	/* mmap that physical buffer */
	enc->virt_bsbuf_addr = IOGetVirtMem(&mem_desc);
	if (enc->virt_bsbuf_addr <= 0) {
		err_msg("Unable to map physical memory\n");
		IOFreePhyMem(&mem_desc);
		free(enc);
		return -1;
	}

	enc->phy_bsbuf_addr = mem_desc.phy_addr;
	enc->cmdl = cmdl;

	if (enc->cmdl->format == STD_MJPG)
		enc->mjpg_fmt = MODE420;  /* Please change this per your needs */

	if (enc->cmdl->mapType) {
                enc->linear2TiledEnable = 1;
		enc->cmdl->chromaInterleave = 1; /* Must be CbCrInterleave for tiled */
		if (cmdl->format == STD_MJPG) {
			err_msg("MJPG encoder cannot support tiled format\n");
			return -1;
		}
        } else
		enc->linear2TiledEnable = 0;

	/* open the encoder */
	ret = encoder_open(enc);
	if (ret)
		goto err;

	/* configure the encoder */
	ret = encoder_configure(enc);
	if (ret)
		goto err1;

        /* allocate scratch buf */
	if (cpu_is_mx6x() && (cmdl->format == STD_MPEG4) && enc->mp4_dataPartitionEnable) {
		scratch_mem_desc.size = MPEG4_SCRATCH_SIZE;
                ret = IOGetPhyMem(&scratch_mem_desc);
                if (ret) {
                        err_msg("Unable to obtain physical slice save mem\n");
                        goto err1;
                }
		enc->scratchBuf.bufferBase = scratch_mem_desc.phy_addr;
		enc->scratchBuf.bufferSize = scratch_mem_desc.size;
        }

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
	if (cpu_is_mx6x() && cmdl->format == STD_MPEG4 && enc->mp4_dataPartitionEnable) {
		IOFreeVirtMem(&scratch_mem_desc);
		IOFreePhyMem(&scratch_mem_desc);
	}
	/* free the physical memory */
	IOFreeVirtMem(&mem_desc);
	IOFreePhyMem(&mem_desc);
	free(enc);
#ifndef COMMON_INIT
	vpu_UnInit();
#endif
	return ret;
}

