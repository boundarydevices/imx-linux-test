/*
 * Copyright 2004-2011 Freescale Semiconductor, Inc.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "vpu_test.h"

#define NUM_FRAME_BUFS	32
#define FB_INDEX_MASK	(NUM_FRAME_BUFS - 1)

static int fb_index;
static struct frame_buf *fbarray[NUM_FRAME_BUFS];
static struct frame_buf fbpool[NUM_FRAME_BUFS];

void framebuf_init(void)
{
	int i;

	for (i = 0; i < NUM_FRAME_BUFS; i++) {
		fbarray[i] = &fbpool[i];
	}
}

struct frame_buf *get_framebuf(void)
{
	struct frame_buf *fb;

	fb = fbarray[fb_index];
	fbarray[fb_index] = 0;

	++fb_index;
	fb_index &= FB_INDEX_MASK;

	return fb;
}

void put_framebuf(struct frame_buf *fb)
{
	--fb_index;
	fb_index &= FB_INDEX_MASK;

	fbarray[fb_index] = fb;
}

struct frame_buf *framebuf_alloc(int stdMode, int format, int strideY, int height, int mvCol)
{
	struct frame_buf *fb;
	int err;
	int divX, divY;

	fb = get_framebuf();
	if (fb == NULL)
		return NULL;

	divX = (format == MODE420 || format == MODE422) ? 2 : 1;
	divY = (format == MODE420 || format == MODE224) ? 2 : 1;

	memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
	fb->desc.size = (strideY * height  + strideY / divX * height / divY * 2);
	if (mvCol)
		fb->desc.size += strideY / divX * height / divY;

	err = IOGetPhyMem(&fb->desc);
	if (err) {
		printf("Frame buffer allocation failure\n");
		memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
		return NULL;
	}

	fb->addrY = fb->desc.phy_addr;
	fb->addrCb = fb->addrY + strideY * height;
	fb->addrCr = fb->addrCb + strideY / divX * height / divY;
	fb->strideY = strideY;
	fb->strideC =  strideY / divX;
	if (mvCol)
		fb->mvColBuf = fb->addrCr + strideY / divX * height / divY;

	fb->desc.virt_uaddr = IOGetVirtMem(&(fb->desc));
	if (fb->desc.virt_uaddr <= 0) {
		IOFreePhyMem(&fb->desc);
		memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
		return NULL;
	}

	return fb;
}

struct frame_buf *tiled_framebuf_alloc(int stdMode, int format, int strideY, int height, int mvCol, int mapType)
{
	struct frame_buf *fb;
	int err, align;
	int divX, divY;
	Uint32 lum_top_base, lum_bot_base, chr_top_base, chr_bot_base;
	Uint32 lum_top_20bits, lum_bot_20bits, chr_top_20bits, chr_bot_20bits;
	int luma_top_size, luma_bot_size, chroma_top_size, chroma_bot_size;

	fb = get_framebuf();
	if (fb == NULL)
		return NULL;

	divX = (format == MODE420 || format == MODE422) ? 2 : 1;
	divY = (format == MODE420 || format == MODE224) ? 2 : 1;

	memset(&(fb->desc), 0, sizeof(vpu_mem_desc));

	/*
	 * The buffers is luma top, chroma top, luma bottom and chroma bottom for
	 * tiled map type, and only 20bits for the address description, so we need
	 * to do 4K page align for each buffer.
	 */
	align = SZ_4K;
	if (mapType == TILED_FRAME_MB_RASTER_MAP) {
		/* luma_top_size means the Y size of one frame, chroma_top_size
		 * means the interleaved UV size of one frame in frame tiled map type*/
		luma_top_size = (strideY * height + align - 1) & ~(align - 1);
		chroma_top_size = (strideY / divX * height / divY * 2 + align - 1) & ~(align - 1);
		luma_bot_size = chroma_bot_size = 0;
	} else {
		/* This is FIELD_FRAME_MB_RASTER_MAP case, there are two fields */
		luma_top_size = (strideY * height / 2 + align - 1) & ~(align - 1);
		luma_bot_size = luma_top_size;
		chroma_top_size = (strideY / divX * height / divY + align - 1) & ~(align - 1);
		chroma_bot_size = chroma_top_size;
	}
	fb->desc.size = luma_top_size + chroma_top_size + luma_bot_size + chroma_bot_size;
	/* There is possible fb->desc.phy_addr in IOGetPhyMem not 4K page align,
	 * so add more SZ_4K byte here for alignment */
	fb->desc.size += align - 1;

	if (mvCol)
		fb->desc.size += strideY / divX * height / divY;

	err = IOGetPhyMem(&fb->desc);
	if (err) {
		printf("Frame buffer allocation failure\n");
		memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
		return NULL;
	}

	fb->desc.virt_uaddr = IOGetVirtMem(&(fb->desc));
	if (fb->desc.virt_uaddr <= 0) {
		IOFreePhyMem(&fb->desc);
		memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
		return NULL;
	}

	lum_top_base = (fb->desc.phy_addr + align - 1) & ~(align -1);
	chr_top_base = lum_top_base + luma_top_size;
	if (mapType == TILED_FRAME_MB_RASTER_MAP) {
		lum_bot_base = 0;
		chr_bot_base = 0;
	} else {
		lum_bot_base = chr_top_base + chroma_top_size;
		chr_bot_base = lum_bot_base + luma_bot_size;
	}

	lum_top_20bits = lum_top_base >> 12;
	lum_bot_20bits = lum_bot_base >> 12;
	chr_top_20bits = chr_top_base >> 12;
	chr_bot_20bits = chr_bot_base >> 12;

	/*
	 * In tiled map format the construction of the buffer pointers is as follows:
	 * 20bit = addrY [31:12]: lum_top_20bits
	 * 20bit = addrY [11: 0], addrCb[31:24]: chr_top_20bits
	 * 20bit = addrCb[23: 4]: lum_bot_20bits
	 * 20bit = addrCb[ 3: 0], addrCr[31:16]: chr_bot_20bits
	 */
	fb->addrY = (lum_top_20bits << 12) + (chr_top_20bits >> 8);
	fb->addrCb = (chr_top_20bits << 24) + (lum_bot_20bits << 4) + (chr_bot_20bits >> 16);
	fb->addrCr = chr_bot_20bits << 16;
	fb->strideY = strideY;
	fb->strideC = strideY / divX;
	if (mvCol)
		fb->mvColBuf = chr_bot_base + chroma_bot_size;

    return fb;
}

void framebuf_free(struct frame_buf *fb)
{
	if (fb == NULL)
		return;

	if (fb->desc.virt_uaddr) {
		IOFreeVirtMem(&fb->desc);
	}

	if (fb->desc.phy_addr) {
		IOFreePhyMem(&fb->desc);
	}

	memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
	put_framebuf(fb);
}

