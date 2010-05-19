/*
 * Copyright 2004-2010 Freescale Semiconductor, Inc.
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

struct frame_buf *framebuf_alloc(int stdMode, int format, int strideY, int height)
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
	if (cpu_is_mx37() || cpu_is_mx5x())
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
	if (cpu_is_mx37() || cpu_is_mx5x()) {
		if (stdMode==STD_MJPG)
			fb->mvColBuf = fb->addrCr;
		else
			fb->mvColBuf = fb->addrCr + strideY / divX * height / divY;
	}
	fb->desc.virt_uaddr = IOGetVirtMem(&(fb->desc));
	if (fb->desc.virt_uaddr <= 0) {
		IOFreePhyMem(&fb->desc);
		memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
		return NULL;
	}

	return fb;
}

void framebuf_free(struct frame_buf *fb)
{
	if (fb->desc.virt_uaddr) {
		IOFreeVirtMem(&fb->desc);
	}

	if (fb->desc.phy_addr) {
		IOFreePhyMem(&fb->desc);
	}

	memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
	put_framebuf(fb);
}

