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

#ifndef _DEC_H
#define _DEC_H

#include <linux/videodev.h>
#include <pthread.h>
#include <errno.h>
#include "vpu_lib.h"
#include "vpu_io.h"

#define	DEBUG_LEVEL	0

#define dprintf(level, fmt, arg...)     if (DEBUG_LEVEL >= level) \
        printf("[DEBUG]\t%s:%d " fmt, __FILE__, __LINE__, ## arg)

#define err_msg(fmt, arg...) do { if (DEBUG_LEVEL >= 1)		\
	printf("[ERR]\t%s:%d " fmt,  __FILE__, __LINE__, ## arg); else \
	printf("[ERR]\t" fmt, ## arg);	\
	} while (0)
#define info_msg(fmt, arg...) do { if (DEBUG_LEVEL >= 1)		\
	printf("[INFO]\t%s:%d " fmt,  __FILE__, __LINE__, ## arg); else \
	printf("[INFO]\t" fmt, ## arg);	\
	} while (0)
#define warn_msg(fmt, arg...) do { if (DEBUG_LEVEL >= 1)		\
	printf("[WARN]\t%s:%d " fmt,  __FILE__, __LINE__, ## arg); else \
	printf("[WARN]\t" fmt, ## arg);	\
	} while (0)

/*#define TVOUT_ENABLE*/
/* Available on MX37. If not defined, then use VPU rotation.  */
#define USE_IPU_ROTATION

typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef signed int s32;
typedef signed short s16;
typedef signed char s8;

#define STREAM_BUF_SIZE		0x40000
#define STREAM_FILL_SIZE	0x8000
#define STREAM_READ_SIZE	(512 * 4)
#define STREAM_END_SIZE		0
#define PS_SAVE_SIZE		0x028000
#define SLICE_SAVE_SIZE		0x02D800

#define STREAM_ENC_PIC_RESET 	1

#define PATH_V4L2	0
#define PATH_FILE	1
#define PATH_NET	2

/* Test operations */
#define ENCODE		1
#define DECODE		2
#define LOOPBACK	3

#define DEFAULT_PORT		5555
#define DEFAULT_PKT_SIZE	40960

struct frame_buf {
	int addrY;
	int addrCb;
	int addrCr;
	int mvColBuf;
	vpu_mem_desc desc;
};

struct v4l_buf {
	void *start;
	off_t offset;
	size_t length;
};

struct vpu_display {
	int fd;
	int nframes;
	int ncount;
	time_t sec;
	suseconds_t usec;
	struct v4l2_buffer buf;
	struct v4l_buf *buffers[32];
} ;

struct capture_testbuffer {
	size_t offset;
	unsigned int length;
};

struct rot {
	int rot_en;
	int ipu_rot_en;
	int rot_angle;
};

#define MAX_PATH	64
struct cmd_line {
	char input[MAX_PATH];	/* Input file name */
	char output[MAX_PATH];  /* Output file name */
	int src_scheme;
	int dst_scheme;
	int src_fd;
	int dst_fd;
	int width;
	int height;
	int format;
	int mp4dblk_en;
	int dering_en;
	int rot_en;
	int ipu_rot_en;
	int rot_angle;
	int mirror;
	int bitrate;
	int gop;
	int save_enc_hdr;
	int count;
	char *nbuf; /* network buffer */
	int nlen; /* remaining data in network buffer */
	int noffset; /* offset into network buffer */
	int seq_no; /* seq numbering to detect skipped frames */
	u16 port; /* udp port number */
	u16 complete; /* wait for the requested buf to be filled completely */
	int iframe;
	pthread_mutex_t *mutex;
};

struct decode {
	DecHandle handle;
	PhysicalAddress phy_bsbuf_addr;
	PhysicalAddress phy_ps_buf;
	PhysicalAddress phy_slice_buf;
	u32 virt_bsbuf_addr;
	int picwidth;
	int picheight;
	int stride;
	int fbcount;
	int extrafb;
	FrameBuffer *fb;
	struct frame_buf **pfbpool;
	struct vpu_display *disp;
	vpu_mem_desc *mvcol_memdesc;
	int reorderEnable;
	int chromaInterleave;
	struct cmd_line *cmdl;
};

struct encode {
	EncHandle handle;		/* Encoder handle */
	PhysicalAddress phy_bsbuf_addr; /* Physical bitstream buffer */
	u32 virt_bsbuf_addr;		/* Virtual bitstream buffer */
	int picwidth;	/* Picture width */
	int picheight;	/* Picture height */
	int fbcount;	/* Total number of framebuffers allocated */
	int src_fbid;	/* Index of frame buffer that contains YUV image */
	FrameBuffer *fb; /* frame buffer base given to encoder */
	struct frame_buf **pfbpool; /* allocated fb pointers are stored here */
	struct cmd_line *cmdl; /* command line */
};

void framebuf_init();
int fwriten(int fd, void *vptr, size_t n);
int freadn(int fd, void *vptr, size_t n);
int vpu_read(struct cmd_line *cmd, char *buf, int n);
int vpu_write(struct cmd_line *cmd, char *buf, int n);
void get_arg(char *buf, int *argc, char *argv[]);
int open_files(struct cmd_line *cmd);
void close_files(struct cmd_line *cmd);
int check_params(struct cmd_line *cmd, int op);
char*skip_unwanted(char *ptr);
int parse_options(char *buf, struct cmd_line *cmd, int *mode);

struct vpu_display *v4l_display_open(int width, int height, int nframes,
					struct rot rotation, int stride);
int v4l_put_data(struct vpu_display *disp);
void v4l_display_close(struct vpu_display *disp);
struct frame_buf *framebuf_alloc(int strideY, int height);
void framebuf_free(struct frame_buf *fb);

int v4l_start_capturing(void);
void v4l_stop_capturing(void);
int v4l_capture_setup(int width, int height, int fps);
int v4l_get_capture_data(struct v4l2_buffer *buf);
void v4l_put_capture_data(struct v4l2_buffer *buf);


int encoder_open(struct encode *enc);
void encoder_close(struct encode *enc);
int encoder_configure(struct encode *enc);
int encoder_allocate_framebuffer(struct encode *enc);
void encoder_free_framebuffer(struct encode *enc);

int decoder_open(struct decode *dec);
int decoder_parse(struct decode *dec);
int decoder_allocate_framebuffer(struct decode *dec);
void decoder_free_framebuffer(struct decode *dec);

static inline void
lock(struct cmd_line *cmdl)
{
	if (cmdl->mutex)
		pthread_mutex_lock(cmdl->mutex);
}

static inline void
unlock(struct cmd_line *cmdl)
{
	if (cmdl->mutex) {
		pthread_mutex_unlock(cmdl->mutex);
		usleep(1);
	}
}

#endif
