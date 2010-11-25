/*
 * pxp_test - test application for the PxP DMA ENGINE lib
 *
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <linux/mxcfb.h>

#include "pxp_lib.h"
#include "pxp_test.h"
#include "fsl_logo_480x360.h"

#define DBG_DEBUG		3
#define DBG_INFO		2
#define DBG_WARNING		1
#define DBG_ERR			0

static int debug_level = DBG_INFO;
#define dbg(flag, fmt, args...)	{ if(flag <= debug_level)  printf("%s:%d "fmt, __FILE__, __LINE__,##args); }

char *usage = "Usage: ./pxp_test.out "\
	       "-I \"<options for each instance>\" "\
	       "-H display this help \n "\
	       "\n"\
	       "options for each instance\n "\
	       "  -o <output file> Write output to file \n "\
	       "	If no output is specified, default is panel \n "\
	       "  -r <rotation angle> \n "\
	       "  -h <horizontal flip> \n "\
	       "  -v <vertical flip> \n "\
	       "  -l <left position for display> \n "\
	       "  -t <top position for display> \n "\
	       "  -i <pixel inversion> \n ";

#define WAVEFORM_MODE_INIT	0x0	/* Screen goes to white (clears) */
#define WAVEFORM_MODE_DU	0x1	/* Grey->white/grey->black */
#define WAVEFORM_MODE_GC16	0x2	/* High fidelity (flashing) */
#define WAVEFORM_MODE_GC4	0x3	/* Lower fidelity (texting, with potential for ghosting) */

#define EPDC_STR_ID		"mxc_epdc_fb"

#if !defined(TEMP_USE_AMBIENT)
#define TEMP_USE_AMBIENT	24
#endif

#define WIDTH	480
#define HEIGHT	360

int fd_fb = 0;
unsigned short * fb0;
int g_fb0_size;

sigset_t sigset;
int quitflag;
unsigned int marker_val = 1;

struct input_argument {
	pthread_t tid;
	char line[256];
	struct cmd_line cmd;
};

sigset_t sigset;
int quitflag;

static struct input_argument input_arg[4];
static int instance;

static char *mainopts = "HI:";
static char *options = "o:r:h:v:l:t:i:";

int parse_main_args(int argc, char *argv[])
{
	int status = 0, opt;

	do {
		opt = getopt(argc, argv, mainopts);
		switch (opt)
		{
		case 'I':
			strncpy(input_arg[instance].line, argv[0], 26);
			strncat(input_arg[instance].line, " ", 2);
			strncat(input_arg[instance].line, optarg, 200);
			instance++;
			break;
		case -1:
			break;
		case 'H':
		default:
			status = -1;
			break;
		}
	} while ((opt != -1) && (status == 0) && (instance < 4));

	optind = 1;
	return status;
}

int parse_args(int argc, char *argv[], int i)
{
	int status = 0, opt;

	do {
		opt = getopt(argc, argv, options);
		switch (opt)
		{
		case 'o':
			strncpy(input_arg[i].cmd.output, optarg, MAX_PATH);
			input_arg[i].cmd.dst_scheme = PATH_FILE;
			break;
		case 'r':
			input_arg[i].cmd.rot_angle = atoi(optarg);
			break;
		case 'h':
			input_arg[i].cmd.hflip = 1;
			break;
		case 'v':
			input_arg[i].cmd.vflip = 1;
			break;
		case 'l':
			input_arg[i].cmd.left = atoi(optarg);
			break;
		case 't':
			input_arg[i].cmd.top = atoi(optarg);
			break;
		case 'i':
			input_arg[i].cmd.pixel_inversion = 1;
			break;
		case -1:
			break;
		default:
			status = -1;
			break;
		}
	} while ((opt != -1) && (status == 0));

	optind = 1;
	return status;
}

static void copy_image_to_fb(int left, int top, int width, int height, uint *img_ptr, struct fb_var_screeninfo *screen_info)
{
	int i;
	uint *fb_ptr =  (uint *)fb0;
	uint bytes_per_pixel;

	if ((width > screen_info->xres) || (height > screen_info->yres)) {
		dbg(DBG_ERR, "Bad image dimensions!\n");
		return;
	}

	bytes_per_pixel = screen_info->bits_per_pixel / 8;

	for (i = 0; i < height; i++) {
		memcpy(fb_ptr + ((i + top) * screen_info->xres_virtual + left) * bytes_per_pixel / 4,
			img_ptr + (i * width) * bytes_per_pixel /4,
			width * bytes_per_pixel);
	}
}

static void update_to_display(int left, int top, int width, int height, int wave_mode, int wait_for_complete)
{
	struct mxcfb_update_data upd_data;
	int retval;

	upd_data.update_mode = UPDATE_MODE_FULL;
	upd_data.waveform_mode = wave_mode;
	upd_data.update_region.left = left;
	upd_data.update_region.width = width;
	upd_data.update_region.top = top;
	upd_data.update_region.height = height;
	upd_data.temp = TEMP_USE_AMBIENT;
	upd_data.flags = 0;

	if (wait_for_complete) {
		/* Get unique marker value */
		upd_data.update_marker = marker_val++;
	} else {
		upd_data.update_marker = 0;
	}

	retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	while (retval < 0 && quitflag == 0) {
		sleep(1);
		retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	}

	if (wait_for_complete) {
		/* Wait for update to complete */
		retval = ioctl(fd_fb, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &upd_data.update_marker);
		if (retval < 0) {
			dbg(DBG_ERR, "Wait for update complete failed.  Error = 0x%x", retval);
		}
	}
}

int pxp_test(void *arg)
{
	struct pxp_config_data *pxp_conf = NULL;
	struct pxp_proc_data *proc_data = NULL;
	int ret = 0, i, n;
	struct pxp_mem_desc mem;
	struct pxp_mem_desc mem_o;
	pxp_chan_handle_t pxp_chan;
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	char fb_dev[10] = "/dev/fb";
	int fb_num = 0;
	int width, height;

	struct cmd_line *cmdl = (struct cmd_line *)arg;

	dbg(DBG_DEBUG, "rot_angle = %d\n", cmdl->rot_angle);
	dbg(DBG_DEBUG, "hflip = %d\n", cmdl->hflip);
	dbg(DBG_DEBUG, "vflip = %d\n", cmdl->vflip);
	dbg(DBG_DEBUG, "display_left = %d\n", cmdl->left);
	dbg(DBG_DEBUG, "display_top = %d\n", cmdl->top);
	dbg(DBG_DEBUG, "pixel_inversion = %d\n", cmdl->pixel_inversion);

	ret = pxp_init();
	if (ret < 0) {
		dbg(DBG_ERR, "pxp init err\n");
		return -1;
	}

	ret = pxp_request_channel(&pxp_chan);
	if (ret < 0) {
		dbg(DBG_ERR, "pxp request channel err\n");
		goto err0;
	}
	dbg(DBG_INFO, "requested chan_id %d\n", pxp_chan.chan_id);

	/* Prepare the channel parameters */
	mem.size = WIDTH * HEIGHT * 2;
	mem_o.size = WIDTH * HEIGHT;

	ret = pxp_get_mem(&mem);
	if (ret < 0) {
		dbg(DBG_DEBUG, "get mem err\n");
		goto err1;
	}
	dbg(DBG_DEBUG, "mem.virt_uaddr %08x, mem.phys_addr %08x, mem.size %d\n",
				mem.virt_uaddr, mem.phys_addr, mem.size);

	ret = pxp_get_mem(&mem_o);
	if (ret < 0) {
		dbg(DBG_ERR, "get mem_o err\n");
		goto err2;
	}

	dbg(DBG_DEBUG, "mem_o.virt_uaddr %08x, mem_o.phys_addr %08x, mem_o.size %d\n",
				mem_o.virt_uaddr, mem_o.phys_addr, mem_o.size);

	for (i = 0; i < (WIDTH * HEIGHT * 2 / 4); i++) {
		*((unsigned int*)mem.virt_uaddr + i) = fb_480x360_2[i];
		if (i < 10)
			dbg(DBG_DEBUG, "[PxP In] 0x%08x 0x%08x\n",
				*((unsigned int *)mem.virt_uaddr + i),
				fb_480x360_2[i]);
	}

	/* Configure the channel */
	pxp_conf = malloc(sizeof (*pxp_conf));
	proc_data = &pxp_conf->proc_data;

	/* Initialize non-channel-specific PxP parameters */
	proc_data->srect.left = 0;
	proc_data->srect.top = 0;
	proc_data->drect.left = 0;
	proc_data->drect.top = 0;
	proc_data->srect.width = WIDTH;
	proc_data->srect.height = HEIGHT;
	proc_data->drect.width =  WIDTH;
	proc_data->drect.height = HEIGHT;
	proc_data->scaling = 0;
	proc_data->hflip = cmdl->hflip;
	proc_data->vflip = cmdl->vflip;
	proc_data->rotate = cmdl->rot_angle;
	proc_data->bgcolor = 0;

	proc_data->overlay_state = 0;
	proc_data->lut_transform = cmdl->pixel_inversion ?
					PXP_LUT_INVERT : PXP_LUT_NONE;

	/*
	 * Initialize S0 parameters
	 */
	pxp_conf->s0_param.pixel_fmt = PXP_PIX_FMT_RGB565;
	pxp_conf->s0_param.width = WIDTH;
	pxp_conf->s0_param.height = HEIGHT;
	pxp_conf->s0_param.color_key = -1;
	pxp_conf->s0_param.color_key_enable = false;
	pxp_conf->s0_param.paddr = mem.phys_addr;

	dbg(DBG_DEBUG, "pxp_test s0 paddr %08x\n", pxp_conf->s0_param.paddr);
	/*
	 * Initialize OL parameters
	 * No overlay will be used for PxP operation
	 */
	 for (i=0; i < 8; i++) {
		pxp_conf->ol_param[i].combine_enable = false;
		pxp_conf->ol_param[i].width = 0;
		pxp_conf->ol_param[i].height = 0;
		pxp_conf->ol_param[i].pixel_fmt = PXP_PIX_FMT_RGB565;
		pxp_conf->ol_param[i].color_key_enable = false;
		pxp_conf->ol_param[i].color_key = -1;
		pxp_conf->ol_param[i].global_alpha_enable = false;
		pxp_conf->ol_param[i].global_alpha = 0;
		pxp_conf->ol_param[i].local_alpha_enable = false;
	}

	/*
	 * Initialize Output channel parameters
	 * Output is Y-only greyscale
	 */
	pxp_conf->out_param.width = WIDTH;
	pxp_conf->out_param.height = HEIGHT;
	pxp_conf->out_param.pixel_fmt = PXP_PIX_FMT_GREY;
	if (cmdl->dst_scheme != PATH_FILE) {
		while (1) {
			fb_dev[7] = '0' + fb_num;
			dbg(DBG_INFO, "opening this fb_dev - %s\n", fb_dev);
			fd_fb = open(fb_dev, O_RDWR);
			if (fd_fb < 0) {
				dbg(DBG_ERR, "Unable to open %s\n", fb_dev);
				goto err3;
			}

			/* Check that fb device is EPDC */
			/* First get screen_info */
			ret = ioctl(fd_fb, FBIOGET_FSCREENINFO, &fix);
			if (ret < 0)
			{
				dbg(DBG_ERR, "Unable to read fixed screeninfo for %s\n", fb_dev);
				goto err3;
			}

			/* If we found EPDC, exit loop */
			if (!strcmp(EPDC_STR_ID, fix.id))
				break;

			fb_num++;
		}
	}

	pxp_conf->out_param.paddr = mem_o.phys_addr;
	dbg(DBG_DEBUG, "pxp_test out paddr %08x\n", pxp_conf->out_param.paddr);

	ret = pxp_config_channel(&pxp_chan, pxp_conf);
	if (ret < 0) {
		dbg(DBG_ERR, "pxp config channel err\n");
		goto err3;
	}

	ret = pxp_start_channel(&pxp_chan);
	if (ret < 0) {
		dbg(DBG_ERR, "pxp start channel err\n");
		goto err3;
	}

	ret = pxp_wait_for_completion(&pxp_chan, 3);
	if (ret < 0) {
		dbg(DBG_ERR, "pxp wait for completion err\n");
		goto err3;
	}

	if (cmdl->dst_scheme != PATH_FILE) {
		var.bits_per_pixel = 8;
		var.xres = 800;
		var.yres = 600;
		var.grayscale = GRAYSCALE_8BIT;
		var.yoffset = 0;
		var.rotate = FB_ROTATE_UR;
		var.activate = FB_ACTIVATE_FORCE;
		ret = ioctl(fd_fb, FBIOPUT_VSCREENINFO, &var);
		if (ret < 0)
		{
			dbg(DBG_ERR, "FBIOPUT_VSCREENINFO error\n");
			goto err4;
		}

		g_fb0_size = var.xres * var.yres * var.bits_per_pixel / 8;
		dbg(DBG_INFO, "g_fb0_size %d\n", g_fb0_size);

		/* Map the device to memory, */
		fb0 = (unsigned short *)mmap(0, g_fb0_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
		if ((int)fb0 <= 0)
		{
			dbg(DBG_ERR, "\nError: failed to map framebuffer device 0 to memory.\n");
			goto err4;
		}
	}

	if (cmdl->dst_scheme == PATH_FILE) {
		n = write(cmdl->dst_fd, (void *)mem_o.virt_uaddr, mem_o.size);
		if (n != mem_o.size) {
			dbg(DBG_DEBUG, "n=%d\n", n);
			dbg(DBG_ERR, "write error\n");
			ret = -1;
			goto err4;
		}
	} else {
		if (cmdl->rot_angle % 180) {
			width = HEIGHT;
			height = WIDTH;
		} else {
			width = WIDTH;
			height = HEIGHT;
		}
		copy_image_to_fb(cmdl->left, cmdl->top,
				 width, height, (void *)mem_o.virt_uaddr, &var);
		dbg(DBG_INFO, "Update to display.\n");
		dbg(DBG_DEBUG, "w/h %d/%d\n", width ,height);
		update_to_display(cmdl->left, cmdl->top,
				  width, height, WAVEFORM_MODE_AUTO, true);
	}

	dbg(DBG_INFO, "pxp_test instance finished!\n");
err4:
	if (cmdl->dst_scheme != PATH_FILE)
	close(fd_fb);
err3:
	pxp_put_mem(&mem_o);
err2:
	pxp_put_mem(&mem);
err1:
	free(pxp_conf);
	pxp_release_channel(&pxp_chan);
err0:
	pxp_uninit();

	return ret;
}

static int signal_thread(void *arg)
{
	int sig, err;

	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	while (1) {
		err = sigwait(&sigset, &sig);
		if (sig == SIGINT) {
			dbg(DBG_INFO, "Ctrl-C received\n");
		} else {
			dbg(DBG_ERR, "Unknown signal. Still exiting\n");
		}
		quitflag = 1;
		break;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	pthread_t sigtid;
	int i, err, nargc;
	char *pargv[32] = {0};

	err = parse_main_args(argc, argv);
	if (err) {
		goto usage;
	}

	if (!instance) {
		goto usage;
	}

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	pthread_create(&sigtid, NULL, (void *)&signal_thread, NULL);

	for (i = 0; i < instance; i++) {
		get_arg(input_arg[i].line, &nargc, pargv);
		err = parse_args(nargc, pargv, i);
		if (err) {
			goto usage;
		}

		if (open_files(&input_arg[i].cmd) == 0) {
			     pthread_create(&input_arg[i].tid,
				   NULL,
				   (void *)&pxp_test,
				   (void *)&input_arg[i].cmd);
		}
	}

	for (i = 0; i < instance; i++) {
		if (input_arg[i].tid != 0) {
			pthread_join(input_arg[i].tid, NULL);
			close_files(&input_arg[i].cmd);
		}
	}

	return 0;
usage:
	printf(usage);
	return -1;
}
