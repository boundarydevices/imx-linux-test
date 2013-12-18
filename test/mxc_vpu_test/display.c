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
#include <linux/mxcfb.h>
#include "vpu_test.h"
#include <linux/ipu.h>
#include <linux/mxc_v4l2.h>

#define V4L2_MXC_ROTATE_NONE                    0
#define V4L2_MXC_ROTATE_VERT_FLIP               1
#define V4L2_MXC_ROTATE_HORIZ_FLIP              2
#define V4L2_MXC_ROTATE_180                     3
#define V4L2_MXC_ROTATE_90_RIGHT                4
#define V4L2_MXC_ROTATE_90_RIGHT_VFLIP          5
#define V4L2_MXC_ROTATE_90_RIGHT_HFLIP          6
#define V4L2_MXC_ROTATE_90_LEFT                 7

int ipu_memory_alloc(int size, int cnt, dma_addr_t paddr[], void * vaddr[], int fd_fb_alloc)
{
	int i, ret = 0;

	for (i=0;i<cnt;i++) {
		/*alloc mem from DMA zone*/
		/*input as request mem size */
		paddr[i] = size;
		if ( ioctl(fd_fb_alloc, FBIO_ALLOC, &(paddr[i])) < 0) {
			printf("Unable alloc mem from /dev/fb0\n");
			close(fd_fb_alloc);
			ret = -1;
			goto done;
		}

		vaddr[i] = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
				fd_fb_alloc, paddr[i]);
		if (vaddr[i] == MAP_FAILED) {
			printf("mmap failed!\n");
			ret = -1;
			goto done;
		}
	}
done:
	return ret;
}

void ipu_memory_free(int size, int cnt, dma_addr_t paddr[], void * vaddr[], int fd_fb_alloc)
{
	int i;

	for (i=0;i<cnt;i++) {
		if (vaddr[i])
			munmap(vaddr[i], size);
		if (paddr[i])
			ioctl(fd_fb_alloc, FBIO_FREE, &(paddr[i]));
	}
}

static pthread_mutex_t ipu_mutex;
static pthread_cond_t ipu_cond;
static int ipu_waiting = 0;
static int ipu_running = 0;
static inline void wait_queue()
{
	pthread_mutex_lock(&ipu_mutex);
	ipu_waiting = 1;
	pthread_cond_wait(&ipu_cond, &ipu_mutex);
	pthread_mutex_unlock(&ipu_mutex);
}

static inline void wakeup_queue()
{
	pthread_cond_signal(&ipu_cond);
}

extern int quitflag;
extern int vpu_v4l_performance_test;

static pthread_mutex_t v4l_mutex;

/* The thread for display in performance test with v4l */
void v4l_disp_loop_thread(void *arg)
{
	struct decode *dec = (struct decode *)arg;
	struct vpu_display *disp = dec->disp;
	pthread_attr_t attr;
	struct timespec ts;
	int error_status = 0, ret;
	struct v4l2_buffer buffer;

	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);

	while (!error_status && !quitflag) {
		/* Use timed wait here */
		do {
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_nsec +=100000000; // 100ms
			if (ts.tv_nsec >= 1000000000)
			{
				ts.tv_sec += ts.tv_nsec / 1000000000;
				ts.tv_nsec %= 1000000000;
			}
		} while ((sem_timedwait(&disp->avaiable_dequeue_frame,
			 &ts) != 0) && !quitflag);

		if (quitflag)
			break;

		buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buffer.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(disp->fd, VIDIOC_DQBUF, &buffer);
		if (ret < 0) {
			err_msg("VIDIOC_DQBUF failed\n");
			error_status = 1;
		}
		queue_buf(&(disp->released_q), buffer.index);

		pthread_mutex_lock(&v4l_mutex);
		disp->queued_count--;
		disp->dequeued_count++;
		pthread_mutex_unlock(&v4l_mutex);
		sem_post(&disp->avaiable_decoding_frame);
	}
	pthread_attr_destroy(&attr);
	return;
}

void ipu_disp_loop_thread(void *arg)
{
	struct decode *dec = (struct decode *)arg;
	DecHandle handle = dec->handle;
	struct vpu_display *disp = dec->disp;
	int index = -1, disp_clr_index, tmp_idx[3] = {0,0,0}, err, mode;
	pthread_attr_t attr;

	ipu_running = 1;

	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);

	while(1) {
		disp_clr_index = index;
		index = dequeue_buf(&(disp->ipu_q));
		if (index < 0) {
			wait_queue();
			ipu_waiting = 0;
			index = dequeue_buf(&(disp->ipu_q));
			if (index < 0) {
				info_msg("thread is going to finish\n");
				break;
			}
		}

		if (disp->ncount == 0) {
			disp->input.user_def_paddr[0] = disp->ipu_bufs[index].ipu_paddr;
			/* For video de-interlace, Low/Medium motion */
			tmp_idx[0] = index;
		} else if ((disp->deinterlaced == 1) && (disp->input.motion_sel != HIGH_MOTION) && (disp->ncount == 1)) {
			disp->input.user_def_paddr[1] = disp->ipu_bufs[index].ipu_paddr;
			/* For video de-interlace, Low/Medium motion */
			tmp_idx[1] = index;
		} else if ((disp->ncount == 1) || ((disp->deinterlaced == 1) && (disp->input.motion_sel != HIGH_MOTION) &&
						 (disp->ncount == 2))) {
			disp->input.user_def_paddr[disp->ncount] = disp->ipu_bufs[index].ipu_paddr;
			mode = (disp->deinterlaced == 1) ? (OP_STREAM_MODE | TASK_VDI_VF_MODE) : (OP_STREAM_MODE | TASK_PP_MODE);
			err = mxc_ipu_lib_task_init(&(disp->input), NULL, &(disp->output), mode, &(disp->ipu_handle));
			if (err < 0) {
				err_msg("mxc_ipu_lib_task_init failed, err %d\n", err);
				quitflag = 1;
				return;
			}
			/* it only enable ipu task and finish first frame */
			err = mxc_ipu_lib_task_buf_update(&(disp->ipu_handle), 0, 0, 0, NULL, NULL);
			if (err < 0) {
				err_msg("mxc_ipu_lib_task_buf_update failed, err %d\n", err);
				quitflag = 1;
				break;
			}
			/* For video de-interlace, Low/Medium motion */
			tmp_idx[2] = index;
			if ((disp->deinterlaced == 1) && (disp->input.motion_sel != HIGH_MOTION))
				disp_clr_index = tmp_idx[0];
		} else {
			err = mxc_ipu_lib_task_buf_update(&(disp->ipu_handle), disp->ipu_bufs[index].ipu_paddr,
					0, 0, NULL, NULL);
			if (err < 0) {
				err_msg("mxc_ipu_lib_task_buf_update failed, err %d\n", err);
				quitflag = 1;
				break;
			}
			/* For video de-interlace, Low/Medium motion */
			if ((disp->deinterlaced == 1) && (disp->input.motion_sel != HIGH_MOTION)) {
				tmp_idx[0] = tmp_idx[1];
				tmp_idx[1] = tmp_idx[2];
				tmp_idx[2] = index;
				disp_clr_index = tmp_idx[0];
			}
		}

		if ((dec->cmdl->format != STD_MJPG) && (disp_clr_index >= 0) && (!disp->stopping) &&
		    !((disp->deinterlaced == 1) && (disp->input.motion_sel != HIGH_MOTION) && (disp->ncount < 2))) {
			err = vpu_DecClrDispFlag(handle, disp_clr_index);
			if (err) {
				err_msg("vpu_DecClrDispFlag failed Error code %d\n", err);
				quitflag = 1;
				break;
			}
		}
		disp->ncount++;
	}
	mxc_ipu_lib_task_uninit(&(disp->ipu_handle));
	pthread_attr_destroy(&attr);
	info_msg("Disp loop thread exit\n");
	ipu_running = 0;
	return;
}

struct vpu_display *
ipu_display_open(struct decode *dec, int nframes, struct rot rotation, Rect cropRect)
{
	int width = dec->picwidth;
	int height = dec->picheight;
	int left = cropRect.left;
	int top = cropRect.top;
	int right = cropRect.right;
	int bottom = cropRect.bottom;
	int disp_width = dec->cmdl->width;
	int disp_height = dec->cmdl->height;
	int disp_left =  dec->cmdl->loff;
	int disp_top =  dec->cmdl->toff;
	char motion_mode = dec->cmdl->vdi_motion;
	int err = 0, i;
	struct vpu_display *disp;
	struct mxcfb_gbl_alpha alpha;
	struct fb_var_screeninfo fb_var;

	disp = (struct vpu_display *)calloc(1, sizeof(struct vpu_display));
	if (disp == NULL) {
		err_msg("falied to allocate vpu_display\n");
		return NULL;
	}

	/* set alpha */
#ifdef BUILD_FOR_ANDROID
	disp->fd = open("/dev/graphics/fb0", O_RDWR, 0);
#else
	disp->fd = open("/dev/fb0", O_RDWR, 0);
#endif
	if (disp->fd < 0) {
		err_msg("unable to open fb0\n");
		free(disp);
		return NULL;
	}
	alpha.alpha = 0;
	alpha.enable = 1;
	if (ioctl(disp->fd, MXCFB_SET_GBL_ALPHA, &alpha) < 0) {
		err_msg("set alpha blending failed\n");
		close(disp->fd);
		free(disp);
		return NULL;
	}
	if ( ioctl(disp->fd, FBIOGET_VSCREENINFO, &fb_var) < 0) {
		err_msg("Get FB var info failed!\n");
		close(disp->fd);
		free(disp);
		return NULL;
	}
	if (!disp_width || !disp_height) {
		disp_width = fb_var.xres;
		disp_height = fb_var.yres;
	}

	if (rotation.rot_en) {
		if (rotation.rot_angle == 90 || rotation.rot_angle == 270) {
			i = width;
			width = height;
			height = i;
		}
		dprintf(3, "VPU rot: width = %d; height = %d\n", width, height);
	}

	/* allocate buffers, use an extra buf for init buf */
	disp->nframes = nframes;
	disp->frame_size = width*height*3/2;
	for (i=0;i<nframes;i++) {
		err = ipu_memory_alloc(disp->frame_size, 1, &(disp->ipu_bufs[i].ipu_paddr),
				&(disp->ipu_bufs[i].ipu_vaddr), disp->fd);
		if ( err < 0) {
			err_msg("ipu_memory_alloc failed\n");
			free(disp);
			return NULL;
		}
	}

	memset(&(disp->ipu_handle), 0, sizeof(ipu_lib_handle_t));
	memset(&(disp->input), 0, sizeof(ipu_lib_input_param_t));
	memset(&(disp->output), 0, sizeof(ipu_lib_output_param_t));

        disp->input.width = width;
        disp->input.height = height;
	disp->input.input_crop_win.pos.x = left;
	disp->input.input_crop_win.pos.y = top;
	disp->input.input_crop_win.win_w = right - left;
	disp->input.input_crop_win.win_h = bottom - top;

	/* Set VDI motion algorithm. */
	if (motion_mode) {
		if (motion_mode == 'h') {
			disp->input.motion_sel = HIGH_MOTION;
		} else if (motion_mode == 'l') {
			disp->input.motion_sel = LOW_MOTION;
		} else if (motion_mode == 'm') {
			disp->input.motion_sel = MED_MOTION;
		} else {
			disp->input.motion_sel = MED_MOTION;
			info_msg("%c unknown motion mode, medium, the default is used\n", motion_mode);
		}
	}

	if (dec->cmdl->chromaInterleave == 0)
		disp->input.fmt = V4L2_PIX_FMT_YUV420;
	else
		disp->input.fmt = V4L2_PIX_FMT_NV12;

	disp->output.width = disp_width;
	disp->output.height = disp_height;
	disp->output.fmt = V4L2_PIX_FMT_UYVY;
	if (rotation.ext_rot_en && (rotation.rot_angle != 0)) {
		if (rotation.rot_angle == 90)
			disp->output.rot = IPU_ROTATE_90_RIGHT;
		else if (rotation.rot_angle == 180)
			disp->output.rot = IPU_ROTATE_180;
		else if (rotation.rot_angle == 270)
			disp->output.rot = IPU_ROTATE_90_LEFT;
	}
	disp->output.fb_disp.pos.x = disp_left;
	disp->output.fb_disp.pos.y = disp_top;
	disp->output.show_to_fb = 1;
	disp->output.fb_disp.fb_num = 2;

	info_msg("Display to %d %d, top offset %d, left offset %d\n",
			disp_width, disp_height, disp_top, disp_left);

	disp->ipu_q.tail = disp->ipu_q.head = 0;
	disp->stopping = 0;

	dec->disp = disp;
	pthread_mutex_init(&ipu_mutex, NULL);
	pthread_cond_init(&ipu_cond, NULL);

	/* start disp loop thread */
	pthread_create(&(disp->ipu_disp_loop_thread), NULL, (void *)ipu_disp_loop_thread, (void *)dec);

	return disp;
}

void ipu_display_close(struct vpu_display *disp)
{
	int i;

	disp->stopping = 1;
	disp->deinterlaced = 0;
	while(ipu_running && ((queue_size(&(disp->ipu_q)) > 0) || !ipu_waiting)) usleep(10000);
	if (ipu_running) {
		wakeup_queue();
		info_msg("Join disp loop thread\n");
		pthread_join(disp->ipu_disp_loop_thread, NULL);
	}
	pthread_mutex_destroy(&ipu_mutex);
	pthread_cond_destroy(&ipu_cond);
	for (i=0;i<disp->nframes;i++)
		ipu_memory_free(disp->frame_size, 1, &(disp->ipu_bufs[i].ipu_paddr),
				&(disp->ipu_bufs[i].ipu_vaddr), disp->fd);
	close(disp->fd);
	free(disp);
}

int ipu_put_data(struct vpu_display *disp, int index, int field, int fps)
{
	/*TODO: ipu lib dose not support fps control yet*/

	disp->ipu_bufs[index].field = field;
	if (field == V4L2_FIELD_TOP || field == V4L2_FIELD_BOTTOM ||
	    field == V4L2_FIELD_INTERLACED_TB ||
	    field == V4L2_FIELD_INTERLACED_BT)
		disp->deinterlaced = 1;
	queue_buf(&(disp->ipu_q), index);
	wakeup_queue();

	return 0;
}

void v4l_free_bufs(int n, struct vpu_display *disp)
{
	int i;
	struct v4l_buf *buf;
	struct v4l_specific_data *v4l_rsd = (struct v4l_specific_data *)disp->render_specific_data;

	for (i = 0; i < n; i++) {
		if (v4l_rsd->buffers[i] != NULL) {
			buf = v4l_rsd->buffers[i];
			if (buf->start > 0)
				munmap(buf->start, buf->length);

			free(buf);
			v4l_rsd->buffers[i] = NULL;
		}
	}
	free(v4l_rsd);
}

static int
calculate_ratio(int width, int height, int maxwidth, int maxheight)
{
	int i, tmp, ratio, compare;

	i = ratio = 1;
	if (width >= height) {
		tmp = width;
		compare = maxwidth;
	} else {
		tmp = height;
		compare = maxheight;
	}

	if (width <= maxwidth && height <= maxheight) {
		ratio = 1;
	} else {
		while (tmp > compare) {
			ratio = (1 << i);
			tmp /= ratio;
			i++;
		}
	}

	return ratio;
}

struct vpu_display *
v4l_display_open(struct decode *dec, int nframes, struct rot rotation, Rect cropRect)
{
	int width = dec->picwidth;
	int height = dec->picheight;
	int left = cropRect.left;
	int top = cropRect.top;
	int right = cropRect.right;
	int bottom = cropRect.bottom;
	int disp_width = dec->cmdl->width;
	int disp_height = dec->cmdl->height;
	int disp_left =  dec->cmdl->loff;
	int disp_top =  dec->cmdl->toff;
	int fd = -1, err = 0, out = 0, i = 0;
	char v4l_device[80], node[8];
	struct v4l2_cropcap cropcap = {0};
	struct v4l2_crop crop = {0};
	struct v4l2_framebuffer fb = {0};
	struct v4l2_format fmt = {0};
	struct v4l2_requestbuffers reqbuf = {0};
	struct v4l2_mxc_offset off = {0};
	struct v4l2_rect icrop = {0};
	struct vpu_display *disp;
	struct v4l_specific_data *v4l_rsd;
	int fd_fb;
	char *tv_mode, *test_mode;
	char motion_mode = dec->cmdl->vdi_motion;
	struct mxcfb_gbl_alpha alpha;

	int ratio = 1;

	if (cpu_is_mx27()) {
		out = 0;
	} else {
		out = 3;
#ifdef BUILD_FOR_ANDROID
		fd_fb = open("/dev/graphics/fb0", O_RDWR, 0);
#else
		fd_fb = open("/dev/fb0", O_RDWR, 0);
#endif
		if (fd_fb < 0) {
			err_msg("unable to open fb0\n");
			return NULL;
		}
		alpha.alpha = 0;
		alpha.enable = 1;
		if (ioctl(fd_fb, MXCFB_SET_GBL_ALPHA, &alpha) < 0) {
			err_msg("set alpha blending failed\n");
			return NULL;
		}
		close(fd_fb);
	}
	dprintf(3, "rot_en:%d; rot_angle:%d; ext_rot_en:%d\n", rotation.rot_en,
			rotation.rot_angle, rotation.ext_rot_en);

	tv_mode = getenv("VPU_TV_MODE");

	if (tv_mode) {
		err = system("/bin/echo 1 > /sys/class/graphics/fb1/blank");
		if (!strcmp(tv_mode, "NTSC")) {
			err = system("/bin/echo U:720x480i-60 > /sys/class/graphics/fb1/mode");
			out = 5;
		} else if (!strcmp(tv_mode, "PAL")) {
			err = system("/bin/echo U:720x576i-50 > /sys/class/graphics/fb1/mode");
			out = 5;
		} else if (!strcmp(tv_mode, "720P")) {
			err = system("/bin/echo U:1280x720p-60 > /sys/class/graphics/fb1/mode");
			out = 5;
		} else {
			out = 3;
			warn_msg("VPU_TV_MODE should be set to NTSC, PAL, or 720P.\n"
				 "\tDefault display is LCD if not set this environment "
				 "or set wrong string.\n");
		}
		err = system("/bin/echo 0 > /sys/class/graphics/fb1/blank");
		if (err == -1) {
			warn_msg("set tv mode error\n");
		}
		/* make sure tvout init done */
		sleep(2);
	}

	if (rotation.rot_en) {
		if (rotation.rot_angle == 90 || rotation.rot_angle == 270) {
			i = width;
			width = height;
			height = i;
		}
		dprintf(3, "VPU rot: width = %d; height = %d\n", width, height);
	}

	disp = (struct vpu_display *)calloc(1, sizeof(struct vpu_display));
       	if (disp == NULL) {
		err_msg("falied to allocate vpu_display\n");
		return NULL;
	}

	if (!dec->cmdl->video_node) {
		if (cpu_is_mx6x())
			dec->cmdl->video_node = 17; /* fg for mx6x */
		else
			dec->cmdl->video_node = 16;
	}
	sprintf(node, "%d", dec->cmdl->video_node);
	strcpy(v4l_device, "/dev/video");
	strcat(v4l_device, node);

	fd = open(v4l_device, O_RDWR, 0);
	if (fd < 0) {
		err_msg("unable to open %s\n", v4l_device);
		goto err;
	}

	info_msg("v4l output to %s\n", v4l_device);

	if (!cpu_is_mx6x()) {
		err = ioctl(fd, VIDIOC_S_OUTPUT, &out);
		if (err < 0) {
			err_msg("VIDIOC_S_OUTPUT failed\n");
			goto err;
		}
	}

	cropcap.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	err = ioctl(fd, VIDIOC_CROPCAP, &cropcap);
	if (err < 0) {
		err_msg("VIDIOC_CROPCAP failed\n");
		goto err;
	}
	dprintf(1, "cropcap.bounds.width = %d\n\tcropcap.bound.height = %d\n\t" \
		"cropcap.defrect.width = %d\n\tcropcap.defrect.height = %d\n",
		cropcap.bounds.width, cropcap.bounds.height,
		cropcap.defrect.width, cropcap.defrect.height);

	if (rotation.ext_rot_en == 0) {
		ratio = calculate_ratio(width, height, cropcap.bounds.width,
				cropcap.bounds.height);
		dprintf(3, "VPU rot: ratio = %d\n", ratio);
	}

	crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	crop.c.top = disp_top;
	crop.c.left = disp_left;
	crop.c.width = width / ratio;
	crop.c.height = height / ratio;

	if ((disp_width != 0) && (disp_height!= 0 )) {
		crop.c.width = disp_width;
		crop.c.height = disp_height;
	} else if (!cpu_is_mx27()) {
		crop.c.width = cropcap.bounds.width;
		crop.c.height = cropcap.bounds.height;
	}

	info_msg("Display to %d %d, top offset %d, left offset %d\n",
			crop.c.width, crop.c.height, disp_top, disp_left);

	dprintf(1, "crop.c.width/height: %d/%d\n", crop.c.width, crop.c.height);

	err = ioctl(fd, VIDIOC_S_CROP, &crop);
	if (err < 0) {
		err_msg("VIDIOC_S_CROP failed\n");
		goto err;
	}

	/* Set VDI motion algorithm. */
	if (motion_mode) {
		struct v4l2_control ctrl;
		ctrl.id = V4L2_CID_MXC_MOTION;
		if (motion_mode == 'h') {
			ctrl.value = HIGH_MOTION;
		} else if (motion_mode == 'l') {
			ctrl.value = LOW_MOTION;
		} else if (motion_mode == 'm') {
			ctrl.value = MED_MOTION;
		} else {
			ctrl.value = MED_MOTION;
			info_msg("%c unknown motion mode, medium, the default is used\n",motion_mode);
		}
		err = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
		if (err < 0) {
			err_msg("VIDIOC_S_CTRL failed\n");
			goto err;
		}
	}

	if (cpu_is_mx6x()) {
		/* Set rotation via new V4L2 interface on 2.6.38 kernel */
		struct v4l2_control ctrl;

		ctrl.id = V4L2_CID_ROTATE;
		if (rotation.ext_rot_en)
			ctrl.value = rotation.rot_angle;
		else
			ctrl.value = 0;
		err = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
		if (err < 0) {
			err_msg("VIDIOC_S_CTRL failed\n");
			goto err;
		}
	} else if (rotation.ext_rot_en && (rotation.rot_angle != 0)) {
		/* Set rotation via V4L2 i/f */
		struct v4l2_control ctrl;
		ctrl.id = V4L2_CID_PRIVATE_BASE;
		if (rotation.rot_angle == 90)
			ctrl.value = V4L2_MXC_ROTATE_90_RIGHT;
		else if (rotation.rot_angle == 180)
			ctrl.value = V4L2_MXC_ROTATE_180;
		else if (rotation.rot_angle == 270)
			ctrl.value = V4L2_MXC_ROTATE_90_LEFT;
		err = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
		if (err < 0) {
			err_msg("VIDIOC_S_CTRL failed\n");
			goto err;
		}
	} else {
		struct v4l2_control ctrl;
		ctrl.id = V4L2_CID_PRIVATE_BASE;
		ctrl.value = 0;
		err = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
		if (err < 0) {
			err_msg("VIDIOC_S_CTRL failed\n");
			goto err;
		}
	}

	if (cpu_is_mx27()) {
		fb.capability = V4L2_FBUF_CAP_EXTERNOVERLAY;
		fb.flags = V4L2_FBUF_FLAG_PRIMARY;

		err = ioctl(fd, VIDIOC_S_FBUF, &fb);
		if (err < 0) {
			err_msg("VIDIOC_S_FBUF failed\n");
			goto err;
		}
	}

	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	/*
	 * Just consider one case:
	 * (top,left) = (0,0)
	 */
	if (top || left) {
		err_msg("This case is not covered in this demo for simplicity:\n"
			"croping rectangle (top, left) != (0, 0); "
			"top/left = %d/%d\n", top, left);
		goto err;
	} else if (right || bottom) {
		if (cpu_is_mx6x()) {
			/* This is aligned with new V4L interface on 2.6.38 kernel */
			fmt.fmt.pix.width = width;
			fmt.fmt.pix.height = height;
			icrop.left = left;
			icrop.top = top;
			icrop.width = right - left;
			icrop.height = bottom - top;
			fmt.fmt.pix.priv =  (unsigned long)&icrop;
		} else {
			fmt.fmt.pix.width = right - left;
			fmt.fmt.pix.height = bottom - top;
			fmt.fmt.pix.bytesperline = width;
			off.u_offset = width * height;
			off.v_offset = off.u_offset + width * height / 4;
			fmt.fmt.pix.priv = (unsigned long) &off;
			fmt.fmt.pix.sizeimage = width * height * 3 / 2;
		}
	} else {
		fmt.fmt.pix.width = width;
		fmt.fmt.pix.height = height;
		fmt.fmt.pix.bytesperline = width;
	}
	dprintf(1, "fmt.fmt.pix.width = %d\n\tfmt.fmt.pix.height = %d\n",
				fmt.fmt.pix.width, fmt.fmt.pix.height);

	fmt.fmt.pix.field = V4L2_FIELD_ANY;

	if (dec->cmdl->mapType == LINEAR_FRAME_MAP) {
		if (dec->cmdl->chromaInterleave == 0) {
			if (dec->mjpg_fmt == MODE420)
				fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
			else if (dec->mjpg_fmt == MODE422)
				fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV422P;
			else {
				err_msg("Display cannot support this MJPG format\n");
				goto err;
			}
		} else {
			if (dec->mjpg_fmt == MODE420) {
				info_msg("Display: NV12\n");
				fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
			}
			else if (dec->mjpg_fmt == MODE422) {
				info_msg("Display: NV16\n");
				fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV16;
			}
			else {
				err_msg("Display cannot support this MJPG format\n");
				goto err;
			}
		}
	}
	else if (dec->cmdl->mapType == TILED_FRAME_MB_RASTER_MAP) {
		fmt.fmt.pix.pixelformat = IPU_PIX_FMT_TILED_NV12;
	}
	else if (dec->cmdl->mapType == TILED_FIELD_MB_RASTER_MAP) {
		fmt.fmt.pix.pixelformat = IPU_PIX_FMT_TILED_NV12F;
	}
	else {
		err_msg("Display cannot support mapType = %d\n", dec->cmdl->mapType);
		goto err;
	}
	err = ioctl(fd, VIDIOC_S_FMT, &fmt);
	if (err < 0) {
		err_msg("VIDIOC_S_FMT failed\n");
		goto err;
	}

	err = ioctl(fd, VIDIOC_G_FMT, &fmt);
	if (err < 0) {
		err_msg("VIDIOC_G_FMT failed\n");
		goto err;
	}

	reqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = nframes;

	err = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
	if (err < 0) {
		err_msg("VIDIOC_REQBUFS failed\n");
		goto err;
	}

	if (reqbuf.count < nframes) {
		err_msg("VIDIOC_REQBUFS: not enough buffers\n");
		goto err;
	}

	v4l_rsd = calloc(1, sizeof(struct v4l_specific_data));
	disp->render_specific_data = (void *)v4l_rsd;

	for (i = 0; i < nframes; i++) {
		struct v4l2_buffer buffer = {0};
		struct v4l_buf *buf;

		buf = calloc(1, sizeof(struct v4l_buf));
		if (buf == NULL) {
			v4l_free_bufs(i, disp);
			goto err;
		}

		v4l_rsd->buffers[i] = buf;

		buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		err = ioctl(fd, VIDIOC_QUERYBUF, &buffer);
		if (err < 0) {
			err_msg("VIDIOC_QUERYBUF: not enough buffers\n");
			v4l_free_bufs(i, disp);
			goto err;
		}
		buf->start = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, buffer.m.offset);

		if (cpu_is_mx6x()) {
			/*
			 * Workaround for new V4L interface change, this change
			 * will be removed after V4L driver is updated for this.
			 * Need to call QUERYBUF ioctl again after mmap.
			 */
			err = ioctl(fd, VIDIOC_QUERYBUF, &buffer);
			if (err < 0) {
				err_msg("VIDIOC_QUERYBUF: not enough buffers\n");
				v4l_free_bufs(i, disp);
				goto err;
			}
		}
		buf->offset = buffer.m.offset;
		buf->length = buffer.length;
		dprintf(3, "V4L2buf phy addr: %08x, size = %d\n",
				    (unsigned int)buf->offset, buf->length);

		if (buf->start == MAP_FAILED) {
			err_msg("mmap failed\n");
			v4l_free_bufs(i, disp);
			goto err;
		}

	}

	disp->fd = fd;
	disp->nframes = nframes;

	/*
	 * Use environment VIDEO_PERFORMANCE_TEST to select different mode.
	 * When doing performance test, video decoding and display are in different
	 * threads and default display fps is controlled by cmd. Display will
	 * show the frame immediately if user doesn't input fps with -a option.
	 * This is different from normal unit test.
	 */
	test_mode = getenv("VIDEO_PERFORMANCE_TEST");
	if (test_mode && !strcmp(test_mode, "1") && (dec->cmdl->dst_scheme == PATH_V4L2))
		vpu_v4l_performance_test = 1;

	if (vpu_v4l_performance_test) {
		dec->disp = disp;
		disp->released_q.tail = disp->released_q.head = 0;
		sem_init(&disp->avaiable_decoding_frame, 0,
			    dec->regfbcount - dec->minfbcount);
		sem_init(&disp->avaiable_dequeue_frame, 0, 0);
		pthread_mutex_init(&v4l_mutex, NULL);
		/* start v4l disp loop thread */
		pthread_create(&(disp->disp_loop_thread), NULL,
				    (void *)v4l_disp_loop_thread, (void *)dec);
	}

	return disp;
err:
	close(fd);
	free(disp);
	return NULL;
}

void v4l_display_close(struct vpu_display *disp)
{
	int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	if (disp) {
		if (vpu_v4l_performance_test) {
			quitflag = 1;
			pthread_join(disp->disp_loop_thread, NULL);
			sem_destroy(&disp->avaiable_decoding_frame);
			sem_destroy(&disp->avaiable_dequeue_frame);
		}

		ioctl(disp->fd, VIDIOC_STREAMOFF, &type);
		v4l_free_bufs(disp->nframes, disp);
		close(disp->fd);
		free(disp);
	}
}

int v4l_get_buf(struct decode *dec)
{
	int index = -1;
	struct timespec ts;
	struct vpu_display *disp;

	disp = dec->disp;
	/* Block here to wait avaiable_decoding_frame */
	if (vpu_v4l_performance_test) {
		do {
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_nsec +=100000000; // 100ms
			if (ts.tv_nsec >= 1000000000)
			{
				ts.tv_sec += ts.tv_nsec / 1000000000;
				ts.tv_nsec %= 1000000000;
			}
		} while ((sem_timedwait(&disp->avaiable_decoding_frame,
			    &ts) != 0) && !quitflag);
	}

	if (disp->dequeued_count > 0) {
		index = dequeue_buf(&(disp->released_q));
		pthread_mutex_lock(&v4l_mutex);
		disp->dequeued_count--;
		pthread_mutex_unlock(&v4l_mutex);
	}
	return index;
}

int v4l_put_data(struct decode *dec, int index, int field, int fps)
{
	struct timeval tv;
	int err, type, threshold;
	struct v4l2_format fmt = {0};
	struct vpu_display *disp;
	struct v4l_specific_data *v4l_rsd;

	disp = dec->disp;
	v4l_rsd = (struct v4l_specific_data *)disp->render_specific_data;

	v4l_rsd->buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	v4l_rsd->buf.memory = V4L2_MEMORY_MMAP;

	/* query buffer info */
	v4l_rsd->buf.index = index;
	err = ioctl(disp->fd, VIDIOC_QUERYBUF, &v4l_rsd->buf);
	if (err < 0) {
		err_msg("VIDIOC_QUERYBUF failed\n");
		goto err;
	}

	if (disp->ncount == 0) {
		gettimeofday(&tv, 0);
		v4l_rsd->buf.timestamp.tv_sec = tv.tv_sec;
		v4l_rsd->buf.timestamp.tv_usec = tv.tv_usec;

		disp->sec = tv.tv_sec;
		disp->usec = tv.tv_usec;
	}

	if (disp->ncount > 0) {
		if (fps != 0) {
			disp->usec += (1000000 / fps);
			if (disp->usec >= 1000000) {
				disp->sec += 1;
				disp->usec -= 1000000;
			}

			v4l_rsd->buf.timestamp.tv_sec = disp->sec;
			v4l_rsd->buf.timestamp.tv_usec = disp->usec;
		} else {
			gettimeofday(&tv, 0);
			v4l_rsd->buf.timestamp.tv_sec = tv.tv_sec;
			v4l_rsd->buf.timestamp.tv_usec = tv.tv_usec;
		}
	}

	v4l_rsd->buf.index = index;
	v4l_rsd->buf.field = field;

	err = ioctl(disp->fd, VIDIOC_QBUF, &v4l_rsd->buf);
	if (err < 0) {
		err_msg("VIDIOC_QBUF failed\n");
		goto err;
	}

	if (vpu_v4l_performance_test) {
		/* Use mutex to protect queued_count in multi-threads */
		pthread_mutex_lock(&v4l_mutex);
		disp->queued_count++;
		pthread_mutex_unlock(&v4l_mutex);
	} else
		disp->queued_count++;

	if (disp->ncount == 1) {
		if ((v4l_rsd->buf.field == V4L2_FIELD_TOP) ||
		    (v4l_rsd->buf.field == V4L2_FIELD_BOTTOM) ||
		    (v4l_rsd->buf.field == V4L2_FIELD_INTERLACED_TB) ||
		    (v4l_rsd->buf.field == V4L2_FIELD_INTERLACED_BT)) {
			/* For interlace feature */
			fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			err = ioctl(disp->fd, VIDIOC_G_FMT, &fmt);
			if (err < 0) {
				err_msg("VIDIOC_G_FMT failed\n");
				goto err;
			}
			if ((v4l_rsd->buf.field == V4L2_FIELD_TOP) ||
			    (v4l_rsd->buf.field == V4L2_FIELD_BOTTOM))
				fmt.fmt.pix.field = V4L2_FIELD_ALTERNATE;
			else
				fmt.fmt.pix.field = field;
			fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			err = ioctl(disp->fd, VIDIOC_S_FMT, &fmt);
			if (err < 0) {
				err_msg("VIDIOC_S_FMT failed\n");
				goto err;
			}
		}
		type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		err = ioctl(disp->fd, VIDIOC_STREAMON, &type);
		if (err < 0) {
			err_msg("VIDIOC_STREAMON failed\n");
			goto err;
		}
	}

	disp->ncount++;

	if (dec->post_processing)
		threshold = dec->rot_buf_count - 1;
	else
		threshold = dec->regfbcount - dec->minfbcount;
	if (disp->queued_count > threshold) {
		if (vpu_v4l_performance_test) {
			sem_post(&disp->avaiable_dequeue_frame);
		} else {
			v4l_rsd->buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			v4l_rsd->buf.memory = V4L2_MEMORY_MMAP;
			err = ioctl(disp->fd, VIDIOC_DQBUF, &v4l_rsd->buf);
			if (err < 0) {
				err_msg("VIDIOC_DQBUF failed\n");
				goto err;
			}
			disp->queued_count--;
		}
	}
	else
		v4l_rsd->buf.index = -1;

	return 0;

err:
	return -1;
}

