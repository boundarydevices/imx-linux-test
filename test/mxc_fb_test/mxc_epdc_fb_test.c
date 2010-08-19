/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/*
 * @file mxc_epdc_fb_test.c
 *
 * @brief Mxc EPDC framebuffer driver unit test application
 *
 */

#ifdef __cplusplus
extern "C"{
#endif

/*=======================================================================
					INCLUDE FILES
=======================================================================*/
/* Standard Include Files */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/mxcfb.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>

#include "ginger_rgb_800x600.c"
#include "fsl_rgb_480x360.c"
#include "colorbar_rgb_800x600.c"


#define TFAIL -1
#define TPASS 0

#define TRUE 1
#define FALSE 0

#define BUFFER_FB		0
#define BUFFER_OVERLAY		1

#define WAVEFORM_MODE_INIT	0x0	/* Screen goes to white (clears) */
#define WAVEFORM_MODE_DU	0x1	/* Grey->white/grey->black */
#define WAVEFORM_MODE_GC16	0x2	/* High fidelity (flashing) */
#define WAVEFORM_MODE_GC4	0x3	/* Lower fidelity */

#define EPDC_STR_ID		"mxc_epdc_fb"

#define  ALIGN_PIXEL_128(x)  ((x+ 127) & ~127)

int fd_fb = 0;
unsigned short * fb;
int g_fb_size;
struct fb_var_screeninfo screen_info;
__u32 marker_val = 1;

#define CROSSHATCH_ALTERNATING  0
#define CROSSHATCH_COLUMNS_ROWS 1

void memset_dword(void *s, int c, size_t count)
{
	int i;
	for (i = 0; i < count; i++)
	{
		*((__u32 *)s + i) = c;
	}
}

static void copy_image_to_buffer(int left, int top, int width, int height, uint *img_ptr,
			uint target_buf, struct fb_var_screeninfo *screen_info)
{
	int i;
	__u32 *dest, *src;
	uint *fb_ptr;

	if (target_buf == BUFFER_FB)
		fb_ptr =  (uint *)fb;
	else if (target_buf == BUFFER_OVERLAY)
		fb_ptr = (uint *)fb +
			(screen_info->xres_virtual * ALIGN_PIXEL_128(screen_info->yres) *
			screen_info->bits_per_pixel/8)/4;
	else {
		printf("Invalid target buffer specified!\n");
		return;
	}

	if ((width > screen_info->xres) || (height > screen_info->yres)) {
		printf("Bad image dimensions!\n");
		return;
	}

	for (i = 0; i < height; i++) {
		dest = fb_ptr + ((i + top) * screen_info->xres_virtual + left) * 2 / 4;
		src = img_ptr + (i * width) * 2 /4;
		memcpy(fb_ptr + ((i + top) * screen_info->xres_virtual + left) * 2 / 4, img_ptr + (i * width) * 2 /4, width * 2);
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
	upd_data.use_alt_buffer = FALSE;

	if (wait_for_complete) {
		/* Get unique marker value */
		upd_data.update_marker = marker_val++;
	} else {
		upd_data.update_marker = 0;
	}

	retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	while (retval < 0) {
		/* We have limited memory available for updates, so wait and
		 * then try again after some updates have completed */
		sleep(1);
		retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	}

/*
	if (wave_mode == WAVEFORM_MODE_AUTO)
		printf("Waveform mode used = %d\n", upd_data.waveform_mode);
*/

	if (wait_for_complete) {
		/* Wait for update to complete */
		retval = ioctl(fd_fb, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &upd_data.update_marker);
		if (retval < 0) {
			printf("Wait for update complete failed.  Error = 0x%x", retval);
		}
	}
}

static void draw_rgb_crosshatch(struct fb_var_screeninfo * var, int mode)
{
	__u32 *stripe_start;
	int i, stripe_cnt;
	__u32 horizontal_color, vertical_color;
	int stripe_width;
	int square_area_side;
	struct mxcfb_update_data upd_data;
	int retval;
	int ioctl_tries;

	/* Draw crossing lines to generate collisions */
	square_area_side = 200;
	horizontal_color = 0x84108410;
	vertical_color = 0x41044104;
	stripe_width = 12;

	if (mode == CROSSHATCH_COLUMNS_ROWS) {
		stripe_cnt = 1;
		while(stripe_cnt * stripe_width * 2 + stripe_width < square_area_side) {

			/* draw vertical strip */
			for (i = 0; i < square_area_side; i++) {

				stripe_start = (__u32 *)fb + ((i * var->xres_virtual)
					+ stripe_width*2*stripe_cnt)/2;

				/* draw stripe */
				memset_dword(stripe_start, horizontal_color, stripe_width / 2);
			}

			upd_data.update_mode = UPDATE_MODE_PARTIAL;
			upd_data.waveform_mode = WAVEFORM_MODE_AUTO;
			upd_data.update_region.left = stripe_width*2*stripe_cnt;
			upd_data.update_region.width = stripe_width;
			upd_data.update_region.top = 0;
			upd_data.update_region.height = square_area_side;
			upd_data.temp = TEMP_USE_AMBIENT;
			upd_data.use_alt_buffer = FALSE;
			ioctl_tries = 0;
			do {
				/* Insert delay after first try */
				if (ioctl_tries > 0) {
					sleep(1);
					printf("Retrying update\n");
				}
				retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
				ioctl_tries++;
			} while ((retval < 0) && (ioctl_tries < 5));
			if (retval < 0)
			{
				printf("Draw crosshatch vertical bar send update failed. retval = %d\n", retval);
			}

			stripe_cnt++;
		}

		stripe_cnt = 1;
		while(stripe_cnt * stripe_width * 2 + stripe_width < square_area_side) {

			/* draw horizontal strip */
			for (i = 0; i < stripe_width; i++) {
				stripe_start = (__u32 *)fb +
					((stripe_width*2*stripe_cnt + i) *
					var->xres_virtual)/2;

				/* draw stripe */
				memset_dword(stripe_start, vertical_color, square_area_side / 2);
			}

			upd_data.update_mode = UPDATE_MODE_PARTIAL;
			upd_data.waveform_mode = WAVEFORM_MODE_AUTO;
			upd_data.update_region.left = 0;
			upd_data.update_region.width = square_area_side;
			upd_data.update_region.top = stripe_width*2*stripe_cnt;
			upd_data.update_region.height = stripe_width;
			upd_data.temp = TEMP_USE_AMBIENT;
			upd_data.use_alt_buffer = FALSE;
			ioctl_tries = 0;
			do {
				/* Insert delay after first try */
				if (ioctl_tries > 0) {
					sleep(1);
					printf("Retrying update\n");
				}
				retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
				ioctl_tries++;
			} while ((retval < 0) && (ioctl_tries < 5));
			if (retval < 0)
			{
				printf("Draw crosshatch horizontal bar send update failed. retval = %d\n", retval);
			}

			stripe_cnt++;
		}
	}

	if (mode == CROSSHATCH_ALTERNATING) {
		stripe_cnt = 1;
		while(stripe_cnt * stripe_width * 2 + stripe_width < square_area_side) {

			/* draw vertical strip */
			for (i = 0; i < square_area_side; i++) {

				stripe_start = (__u32 *)fb + ((i * var->xres_virtual)
					+ stripe_width*2*stripe_cnt)/2;

				/* draw stripe */
				memset_dword(stripe_start, horizontal_color, stripe_width / 2);
			}

			upd_data.update_marker = 0;

			upd_data.update_mode = UPDATE_MODE_PARTIAL;
			upd_data.waveform_mode = WAVEFORM_MODE_AUTO;
			upd_data.update_region.left = stripe_width*2*stripe_cnt;
			upd_data.update_region.width = stripe_width;
			upd_data.update_region.top = 0;
			upd_data.update_region.height = square_area_side;
			upd_data.temp = TEMP_USE_AMBIENT;
			upd_data.use_alt_buffer = FALSE;
			ioctl_tries = 0;
			do {
				/* Insert delay after first try */
				if (ioctl_tries > 0) {
					sleep(1);
					printf("Retrying update\n");
				}
				retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
				ioctl_tries++;
			} while ((retval < 0) && (ioctl_tries < 5));
			if (retval < 0)
			{
				printf("Draw crosshatch vertical bar send update failed. retval = %d\n", retval);
			}

			/* draw horizontal strip */
			for (i = 0; i < stripe_width; i++) {
				stripe_start = (__u32 *)fb +
					((stripe_width*2*stripe_cnt + i) *
					var->xres_virtual)/2;

				/* draw stripe */
				memset_dword(stripe_start, vertical_color, square_area_side / 2);
			}

			upd_data.update_marker = 0;

			upd_data.update_mode = UPDATE_MODE_PARTIAL;
			upd_data.waveform_mode = WAVEFORM_MODE_AUTO;
			upd_data.update_region.left = 0;
			upd_data.update_region.width = square_area_side;
			upd_data.update_region.top = stripe_width*2*stripe_cnt;
			upd_data.update_region.height = stripe_width;
			upd_data.temp = TEMP_USE_AMBIENT;
			upd_data.use_alt_buffer = FALSE;
			ioctl_tries = 0;
			do {
				/* Insert delay after first try */
				if (ioctl_tries > 0) {
					sleep(1);
					printf("Retrying update\n");
				}
				retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
				ioctl_tries++;
			} while ((retval < 0) && (ioctl_tries < 5));
			if (retval < 0)
			{
				printf("Draw crosshatch horizontal bar send update failed. retval = %d\n", retval);
			}

			stripe_cnt++;
		}
	}
}

static void draw_rgb_color_squares(struct fb_var_screeninfo * var)
{
	int i, j;
	int *fbp = (int *)fb;

	/* Draw red square */
	for (i = 50; i < 250; i++)
		for (j = 50; j < 250; j += 2)
			fbp[(i*var->xres_virtual+j)*2/4] = 0xF800F800;

	/* Draw green square */
	for (i = 50; i < 250; i++)
		for (j = 350; j < 550; j += 2)
			fbp[(i*var->xres_virtual+j)*2/4] = 0x07E007E0;

	/* Draw blue square */
	for (i = 350; i < 550; i++)
		for (j = 50; j < 250; j += 2)
			fbp[(i*var->xres_virtual+j)*2/4] = 0x001F001F;

	/* Draw black square */
	for (i = 350; i < 550; i++)
		for (j = 350; j < 550; j += 2)
			fbp[(i*var->xres_virtual+j)*2/4] = 0x00000000;
}

static void draw_y_colorbar(struct fb_var_screeninfo * var)
{
	uint *upd_buf_ptr, *stripe_start;
	int i, stripe_cnt;
	__u32 grey_val, grey_increment;
	__u32 grey_dword;
	int num_stripes, stripe_length, stripe_width;

	upd_buf_ptr = (uint *)fb;

	num_stripes = 16;
	stripe_width = var->xres / num_stripes;
	stripe_width += (4 - stripe_width % 4) % 4;
	grey_increment = 0x100 / num_stripes;

	grey_val = 0x00;
	grey_dword = 0x00000000;

	/*
	 * Generate left->right color bar
	 */
	for (stripe_cnt = 0; stripe_cnt < num_stripes; stripe_cnt++) {
		for (i = 0; i < var->yres; i++) {
			stripe_start =
			    upd_buf_ptr + ((i * var->xres_virtual) +
					   (stripe_width * stripe_cnt)) / 4;
			if ((stripe_width * (stripe_cnt + 1)) > var->xres)
				stripe_length =
				    var->xres - (stripe_width * stripe_cnt);
			else
				stripe_length = stripe_width;

			grey_dword =
			    grey_val | (grey_val << 8) | (grey_val << 16) |
			    (grey_val << 24);

			/* draw stripe */
			memset_dword(stripe_start, grey_dword,
				    stripe_length / 4);
		}
		/* increment grey value to darken next stripe */
		grey_val += grey_increment;
	}
}

static int test_updates(void)
{
	printf("Blank screen\n");
	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
	update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_DU, TRUE);

	printf("Crosshatch updates\n");
	draw_rgb_crosshatch(&screen_info, CROSSHATCH_ALTERNATING);

	sleep(3);

	printf("Blank screen\n");
	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
	update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_DU, TRUE);

	printf("Squares update\n");
	draw_rgb_color_squares(&screen_info);

	/* Update each square */
	update_to_display(50, 50, 200, 200, WAVEFORM_MODE_AUTO, TRUE);
	update_to_display(350, 50, 200, 200, WAVEFORM_MODE_AUTO, TRUE);
	update_to_display(50, 350, 200, 200, WAVEFORM_MODE_AUTO, TRUE);
	update_to_display(350, 350, 200, 200, WAVEFORM_MODE_AUTO, TRUE);

	printf("Blank screen\n");
	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
	update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_DU, TRUE);

	printf("FSL updates\n");
	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
	copy_image_to_buffer(300, 0, 480, 360, fsl_rgb_480x360, BUFFER_FB, &screen_info);
	update_to_display(300, 0, 480, 560, WAVEFORM_MODE_AUTO, TRUE);

	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
	copy_image_to_buffer(300, 48, 480, 360, fsl_rgb_480x360, BUFFER_FB, &screen_info);
	update_to_display(300, 0, 480, 560, WAVEFORM_MODE_AUTO, TRUE);

	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
	copy_image_to_buffer(300, 100, 480, 360, fsl_rgb_480x360, BUFFER_FB, &screen_info);
	update_to_display(300, 0, 480, 560, WAVEFORM_MODE_AUTO, TRUE);

	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
	copy_image_to_buffer(300, 148, 480, 360, fsl_rgb_480x360, BUFFER_FB, &screen_info);
	update_to_display(300, 0, 480, 560, WAVEFORM_MODE_AUTO, TRUE);

	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
	copy_image_to_buffer(300, 200, 480, 360, fsl_rgb_480x360, BUFFER_FB, &screen_info);
	update_to_display(300, 0, 480, 560, WAVEFORM_MODE_AUTO, TRUE);

	printf("Ginger update\n");
	copy_image_to_buffer(0, 0, 800, 600, ginger_rgb_800x600, BUFFER_FB, &screen_info);
	update_to_display(0, 0, 800, 600, WAVEFORM_MODE_AUTO, FALSE);

	sleep(3);

	printf("Colorbar update\n");
	copy_image_to_buffer(0, 0, 800, 600, colorbar_rgb_800x600, BUFFER_FB, &screen_info);
	update_to_display(0, 0, 800, 600, WAVEFORM_MODE_AUTO, FALSE);

	sleep(3);

	return TPASS;
}

static int test_rotation(void)
{
	int retval = TPASS;
	int i;

	printf("Blank screen\n");
	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
	update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_DU, TRUE);

	for (i = FB_ROTATE_CW; i <= FB_ROTATE_CCW; i++) {
		printf("Rotating FB 90 degrees\n");
		screen_info.rotate = i;
		retval = ioctl(fd_fb, FBIOPUT_VSCREENINFO, &screen_info);
		if (retval < 0)
		{
			printf("Rotation failed\n");
			return TFAIL;
		}

		printf("New dimensions: xres = %d, xres_virtual = %d,"
			"yres = %d, yres_virtual = %d\n",
			screen_info.xres, screen_info.xres_virtual,
			screen_info.yres, screen_info.yres_virtual);

		printf("Rotated FSL\n");
		copy_image_to_buffer(0, 0, 480, 360, fsl_rgb_480x360, BUFFER_FB, &screen_info);
		update_to_display(0, 0, 480, 360, WAVEFORM_MODE_AUTO, FALSE);

		sleep(3);
		printf("Blank screen\n");
		memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
		update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_DU, TRUE);

		printf("Rotated squares\n");
		draw_rgb_color_squares(&screen_info);
		update_to_display(50, 50, 200, 200, WAVEFORM_MODE_AUTO, TRUE);
		update_to_display(350, 50, 200, 200, WAVEFORM_MODE_AUTO, TRUE);
		update_to_display(50, 350, 200, 200, WAVEFORM_MODE_AUTO, TRUE);
		update_to_display(350, 350, 200, 200, WAVEFORM_MODE_AUTO, TRUE);

		sleep(2);

		printf("Blank screen\n");
		memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
		update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_DU, TRUE);

		printf("Rotated crosshatch updates\n");
		draw_rgb_crosshatch(&screen_info, CROSSHATCH_COLUMNS_ROWS);

		sleep(4);

		printf("Blank screen\n");
		memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
		update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_DU, TRUE);
	}

	printf("Return rotation to 0\n");
	screen_info.rotate = FB_ROTATE_UR;
	retval = ioctl(fd_fb, FBIOPUT_VSCREENINFO, &screen_info);
	if (retval < 0)
	{
		printf("Rotation failed\n");
		return TFAIL;
	}

	return TPASS;
}

static int test_y8(void)
{
	int retval = TPASS;
	int iter;

	printf("Blank screen\n");
	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
	update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_GC16, TRUE);

	/*
	 * Do Y8 FB test sequence twice:
	 * - once for normal Y8 (grayscale = 1)
	 * - once for inverted Y8 (grayscale = 2)
	 */
	for (iter = 1; iter < 3; iter++) {
		if (iter == GRAYSCALE_8BIT)
			printf("Changing to Y8 Framebuffer\n");
		else if (iter == GRAYSCALE_8BIT_INVERTED)
			printf("Changing to Inverted Y8 Framebuffer\n");
		screen_info.rotate = FB_ROTATE_CW;
		screen_info.bits_per_pixel = 8;
		screen_info.grayscale = iter;
		retval = ioctl(fd_fb, FBIOPUT_VSCREENINFO, &screen_info);
		if (retval < 0)
		{
			printf("Rotate and go to Y8 failed\n");
			return TFAIL;
		}

		printf("Top-half black\n");
		memset(fb, 0x00, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8/2);
		update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_AUTO, FALSE);

		sleep(3);

		printf("Draw Y8 colorbar\n");
		draw_y_colorbar(&screen_info);
		update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_AUTO, FALSE);

		sleep(3);
	}

	printf("Change back to non-inverted RGB565\n");
	screen_info.rotate = FB_ROTATE_UR;
	screen_info.bits_per_pixel = 16;
	screen_info.grayscale = 0;
	retval = ioctl(fd_fb, FBIOPUT_VSCREENINFO, &screen_info);
	if (retval < 0)
	{
		printf("Back to normal failed\n");
		return TFAIL;
	}

	return TPASS;
}

static int test_auto_waveform(void)
{
	int retval = TPASS;
	int i, j;

	printf("Change to Y8 FB\n");
	screen_info.rotate = FB_ROTATE_UR;
	screen_info.bits_per_pixel = 8;
	screen_info.grayscale = GRAYSCALE_8BIT;
	retval = ioctl(fd_fb, FBIOPUT_VSCREENINFO, &screen_info);
	if (retval < 0)
	{
		printf("Back to normal failed\n");
		return TFAIL;
	}

	printf("Blank screen - auto-selected to DU\n");
	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
	update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_AUTO, TRUE);

	/* Draw 0x5 square */
	for (i = 10; i < 50; i++)
		for (j = 10; j < 50; j += 2)
			*((__u32 *)fb + (i*screen_info.xres_virtual+j)*2/4) = 0x50505050;

	/* Draw 0xA square */
	for (i = 60; i < 100; i++)
		for (j = 60; j < 100; j += 2)
			*((__u32 *)fb + (i*screen_info.xres_virtual+j)*2/4) = 0xA0A0A0A0;

	printf("Update auto-selected to GC4\n");
	update_to_display(0, 0, 100, 100, WAVEFORM_MODE_AUTO, FALSE);

	sleep(2);

	/* Taint the GC4 region */
	fb[5] = 0x8080;

	printf("Update auto-selected to GC16\n");
	update_to_display(0, 0, 100, 100, WAVEFORM_MODE_AUTO, FALSE);

	sleep(3);

	printf("Blank screen\n");
	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
	update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_DU, TRUE);

	sleep(2);
	printf("Back to unrotated RGB565\n");
	screen_info.rotate = FB_ROTATE_UR;
	screen_info.bits_per_pixel = 16;
	screen_info.grayscale = 0;
	retval = ioctl(fd_fb, FBIOPUT_VSCREENINFO, &screen_info);
	if (retval < 0)
	{
		printf("Setting RGB565 failed\n");
		return TFAIL;
	}

	return TPASS;
}

static int test_auto_update(void)
{
	int retval = TPASS;
	int auto_update_mode;
	int i;

	printf("Blank screen\n");
	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);
	update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_DU, TRUE);

	printf("Change to auto-update mode\n");
	auto_update_mode = AUTO_UPDATE_MODE_AUTOMATIC_MODE;
	retval = ioctl(fd_fb, MXCFB_SET_AUTO_UPDATE_MODE, &auto_update_mode);
	if (retval < 0)
	{
		return TFAIL;
	}

	printf("Auto-update 1st 5 lines\n");
	for (i = 0; i < 5; i++) {
		memset(fb, 0x00, screen_info.xres_virtual*5*screen_info.bits_per_pixel/8);
	}

	sleep(1);

	printf("Auto-update middle and lower stripes\n");
	for (i = 0; i < 5; i++)
		memset(fb + screen_info.xres_virtual*300*screen_info.bits_per_pixel/8/2, 0x00, screen_info.xres_virtual*5*screen_info.bits_per_pixel/8);
	for (i = 0; i < 5; i++)
		memset(fb + screen_info.xres_virtual*500*screen_info.bits_per_pixel/8/2, 0x00, screen_info.xres_virtual*5*screen_info.bits_per_pixel/8);

	sleep(1);
	printf("Auto-update blank screen\n");
	memset(fb, 0xFF, screen_info.xres_virtual*screen_info.yres*screen_info.bits_per_pixel/8);

	sleep(1);

	printf("Auto-update FSL logo\n");
	copy_image_to_buffer(0, 0, 480, 360, fsl_rgb_480x360, BUFFER_FB, &screen_info);

	sleep(2);

	printf("Change to region update mode\n");
	auto_update_mode = AUTO_UPDATE_MODE_REGION_MODE;
	retval = ioctl(fd_fb, MXCFB_SET_AUTO_UPDATE_MODE, &auto_update_mode);
	if (retval < 0)
	{
		return TFAIL;
	}

	sleep(2);
	printf("Return rotation to 0 degrees\n");
	screen_info.rotate = FB_ROTATE_UR;
	screen_info.bits_per_pixel = 16;
	screen_info.grayscale = 0;
	retval = ioctl(fd_fb, FBIOPUT_VSCREENINFO, &screen_info);
	if (retval < 0)
	{
		printf("Rotation failed\n");
		return TFAIL;
	}

	return TPASS;
}


static int test_pan(void)
{
	int y;
	int retval;

	printf("Draw to offscreen region.\n");
	copy_image_to_buffer(0, 0, 800, 600, colorbar_rgb_800x600,
		BUFFER_OVERLAY, &screen_info);

	printf("Ginger update\n");
	copy_image_to_buffer(0, 0, 800, 600, ginger_rgb_800x600, BUFFER_FB,
		&screen_info);
	update_to_display(0, 0, 800, 600, WAVEFORM_MODE_AUTO, TRUE);

	for (y = 0; y + screen_info.yres <= screen_info.yres_virtual; y+=50) {
		screen_info.yoffset = y;
		retval = ioctl(fd_fb, FBIOPAN_DISPLAY, &screen_info);
		if (retval < 0) {
			printf("Pan fail!\n");
			break;
		}
	}
	printf("Pan test done.\n");

	screen_info.yoffset = 0;
	retval = ioctl(fd_fb, FBIOPAN_DISPLAY, &screen_info);
	if (retval < 0)
		printf("Pan fail!\n");

	printf("Returned to original position.\n");

	sleep(1);

	return TPASS;
}


static int test_overlay(void)
{
	struct mxcfb_update_data upd_data;
	int retval;
	struct fb_fix_screeninfo fix_screen_info;
	__u32 ol_phys_addr;

	printf("Ginger update\n");
	copy_image_to_buffer(0, 0, 800, 600, ginger_rgb_800x600, BUFFER_FB,
		&screen_info);
	update_to_display(0, 0, 800, 600, WAVEFORM_MODE_AUTO, TRUE);

	sleep(1);

	retval = ioctl(fd_fb, FBIOGET_FSCREENINFO, &fix_screen_info);
	if (retval < 0)
		return TFAIL;

	/* Fill overlay buffer with data */
	copy_image_to_buffer(0, 0, 800, 600, colorbar_rgb_800x600,
		BUFFER_OVERLAY, &screen_info);

	ol_phys_addr = fix_screen_info.smem_start +
		screen_info.xres_virtual*ALIGN_PIXEL_128(screen_info.yres)*screen_info.bits_per_pixel/8;

	upd_data.update_mode = UPDATE_MODE_FULL;
	upd_data.waveform_mode = WAVEFORM_MODE_AUTO;
	upd_data.update_region.left = 0;
	upd_data.update_region.width = screen_info.xres;
	upd_data.update_region.top = 0;
	upd_data.update_region.height = screen_info.yres;
	upd_data.temp = TEMP_USE_AMBIENT;
	upd_data.update_marker = marker_val++;

	upd_data.use_alt_buffer = TRUE;
	/* Overlay buffer data */
	upd_data.alt_buffer_data.phys_addr = ol_phys_addr;
	upd_data.alt_buffer_data.width = screen_info.xres_virtual;
	upd_data.alt_buffer_data.height = screen_info.yres;
	upd_data.alt_buffer_data.alt_update_region.left = 0;
	upd_data.alt_buffer_data.alt_update_region.width = screen_info.xres;
	upd_data.alt_buffer_data.alt_update_region.top = 0;
	upd_data.alt_buffer_data.alt_update_region.height = screen_info.yres;

	printf("Show full-screen overlay\n");

	retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	while (retval < 0) {
		/* We have limited memory available for updates, so wait and
		 * then try again after some updates have completed */
		sleep(1);
		retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	}

	/* Wait for update to complete */
	retval = ioctl(fd_fb, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &upd_data.update_marker);
	if (retval < 0) {
		printf("Wait for update complete failed.  Error = 0x%x", retval);
	}

	sleep(2);

	printf("Show FB contents again\n");

	update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_AUTO, TRUE);

	sleep(1);

	printf("Show top half overlay\n");

	/* Update region of overlay shown */
	upd_data.update_region.left = 0;
	upd_data.update_region.width = screen_info.xres;
	upd_data.update_region.top = 0;
	upd_data.update_region.height = screen_info.yres/2;
	upd_data.update_marker = marker_val++;
	upd_data.alt_buffer_data.alt_update_region.left = 0;
	upd_data.alt_buffer_data.alt_update_region.width = screen_info.xres;
	upd_data.alt_buffer_data.alt_update_region.top = 0;
	upd_data.alt_buffer_data.alt_update_region.height = screen_info.yres/2;

	retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	while (retval < 0) {
		/* We have limited memory available for updates, so wait and
		 * then try again after some updates have completed */
		sleep(1);
		retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	}

	/* Wait for update to complete */
	retval = ioctl(fd_fb, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &upd_data.update_marker);
	if (retval < 0) {
		printf("Wait for update complete failed.  Error = 0x%x", retval);
	}

	sleep(2);

	printf("Show FB contents again\n");

	update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_AUTO, TRUE);

	sleep(1);

	printf("Show cutout region of overlay\n");

	/* Update region of overlay shown */
	upd_data.update_region.left = screen_info.xres/4;
	upd_data.update_region.width = screen_info.xres/2;
	upd_data.update_region.top = screen_info.yres/4;
	upd_data.update_region.height = screen_info.yres/2;
	upd_data.update_marker = marker_val++;
	upd_data.alt_buffer_data.alt_update_region.left = screen_info.xres/4;
	upd_data.alt_buffer_data.alt_update_region.width = screen_info.xres/2;
	upd_data.alt_buffer_data.alt_update_region.top = screen_info.yres/4;
	upd_data.alt_buffer_data.alt_update_region.height = screen_info.yres/2;

	retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	while (retval < 0) {
		/* We have limited memory available for updates, so wait and
		 * then try again after some updates have completed */
		sleep(1);
		retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	}

	/* Wait for update to complete */
	retval = ioctl(fd_fb, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &upd_data.update_marker);
	if (retval < 0) {
		printf("Wait for update complete failed.  Error = 0x%x", retval);
	}

	sleep(1);

	printf("Show cutout in upper corner\n");

	/* Update region of overlay shown */
	upd_data.update_region.left = 0;
	upd_data.update_region.width = screen_info.xres/2;
	upd_data.update_region.top = 0;
	upd_data.update_region.height = screen_info.yres/2;
	upd_data.update_marker = marker_val++;
	upd_data.alt_buffer_data.alt_update_region.left = screen_info.xres/4;
	upd_data.alt_buffer_data.alt_update_region.width = screen_info.xres/2;
	upd_data.alt_buffer_data.alt_update_region.top = screen_info.yres/4;
	upd_data.alt_buffer_data.alt_update_region.height = screen_info.yres/2;

	retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	while (retval < 0) {
		/* We have limited memory available for updates, so wait and
		 * then try again after some updates have completed */
		sleep(1);
		retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	}

	/* Wait for update to complete */
	retval = ioctl(fd_fb, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &upd_data.update_marker);
	if (retval < 0) {
		printf("Wait for update complete failed.  Error = 0x%x", retval);
	}

	sleep(1);


	printf("Test overlay at 90 degree rotation\n");
	screen_info.rotate = FB_ROTATE_CW;
	retval = ioctl(fd_fb, FBIOPUT_VSCREENINFO, &screen_info);
	if (retval < 0)
	{
		printf("Rotation failed\n");
		return TFAIL;
	}

	/* FB to black */
	printf("Background to black\n");
	memset(fb, 0x00, screen_info.xres*screen_info.yres*screen_info.bits_per_pixel/8);
	update_to_display(0, 0, screen_info.xres, screen_info.yres, WAVEFORM_MODE_AUTO, TRUE);

	printf("Show rotated overlay in center\n");
	copy_image_to_buffer(0, 0, 480, 360, fsl_rgb_480x360,
		BUFFER_OVERLAY, &screen_info);

	ol_phys_addr = fix_screen_info.smem_start +
		screen_info.xres_virtual*ALIGN_PIXEL_128(screen_info.yres)*screen_info.bits_per_pixel/8;
	upd_data.alt_buffer_data.phys_addr = ol_phys_addr;
	upd_data.alt_buffer_data.width = screen_info.xres_virtual;
	upd_data.alt_buffer_data.height = screen_info.yres;

	/* Update region of overlay shown */
	upd_data.update_region.left = (screen_info.xres - 480)/2;
	upd_data.update_region.width = 480;
	upd_data.update_region.top = (screen_info.yres - 360)/2;
	upd_data.update_region.height = 360;
	upd_data.update_marker = marker_val++;
	upd_data.alt_buffer_data.alt_update_region.left = 0;
	upd_data.alt_buffer_data.alt_update_region.width = 480;
	upd_data.alt_buffer_data.alt_update_region.top = 0;
	upd_data.alt_buffer_data.alt_update_region.height = 360;

	retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	while (retval < 0) {
		/* We have limited memory available for updates, so wait and
		 * then try again after some updates have completed */
		sleep(1);
		retval = ioctl(fd_fb, MXCFB_SEND_UPDATE, &upd_data);
	}

	/* Wait for update to complete */
	retval = ioctl(fd_fb, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &upd_data.update_marker);
	if (retval < 0) {
		printf("Wait for update complete failed.  Error = 0x%x", retval);
	}

	sleep(3);

	printf("Revert rotation\n");
	screen_info.rotate = FB_ROTATE_UR;
	retval = ioctl(fd_fb, FBIOPUT_VSCREENINFO, &screen_info);
	if (retval < 0)
	{
		printf("Rotation failed\n");
		return TFAIL;
	}

	return TPASS;
}

int
main(int argc, char **argv)
{
	int retval = TPASS;
	int auto_update_mode;
	struct mxcfb_waveform_modes wv_modes;
	char fb_dev[10] = "/dev/fb";
	int fb_num = 0;
	struct fb_fix_screeninfo screen_info_fix;

	/* Find EPDC FB device */
	while (1) {
		fb_dev[7] = '0' + fb_num;
		fd_fb = open(fb_dev, O_RDWR, 0);
		if (fd_fb < 0) {
			printf("Unable to open %s\n", fb_dev);
			retval = TFAIL;
			goto err0;
		}

		/* Check that fb device is EPDC */
		/* First get screen_info */
		retval = ioctl(fd_fb, FBIOGET_FSCREENINFO, &screen_info_fix);
		if (retval < 0)
		{
			printf("Unable to read fixed screeninfo for %s\n", fb_dev);
			goto err1;
		}

		/* If we found EPDC, exit loop */
		if (!strcmp(EPDC_STR_ID, screen_info_fix.id))
			break;

		fb_num++;
	}

	printf("Opened EPDC fb device %s\n", fb_dev);

	retval = ioctl(fd_fb, FBIOGET_VSCREENINFO, &screen_info);
	if (retval < 0)
	{
		goto err1;
	}
	printf("Set the background to 16-bpp\n");
	screen_info.bits_per_pixel = 16;
	screen_info.grayscale = 0;
	screen_info.yoffset = 0;
	screen_info.rotate = FB_ROTATE_UR;
	screen_info.activate = FB_ACTIVATE_FORCE;
	retval = ioctl(fd_fb, FBIOPUT_VSCREENINFO, &screen_info);
	if (retval < 0)
	{
		goto err1;
	}
	g_fb_size = screen_info.xres_virtual * screen_info.yres_virtual * screen_info.bits_per_pixel / 8;

	printf("screen_info.xres_virtual = %d\nscreen_info.yres_virtual = %d\nscreen_info.bits_per_pixel = %d\n",
		screen_info.xres_virtual, screen_info.yres_virtual, screen_info.bits_per_pixel);

	printf("Mem-Mapping FB0\n");

	/* Map the device to memory*/
	fb = (unsigned short *)mmap(0, g_fb_size,PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
	if ((int)fb <= 0)
	{
		printf("\nError: failed to map framebuffer device 0 to memory.\n");
		goto err1;
	}

	printf("Set to region update mode\n");
	auto_update_mode = AUTO_UPDATE_MODE_REGION_MODE;
	retval = ioctl(fd_fb, MXCFB_SET_AUTO_UPDATE_MODE, &auto_update_mode);
	if (retval < 0)
	{
		goto err2;
	}

	printf("Set waveform modes\n");
	wv_modes.mode_init = 0;
	wv_modes.mode_du = 1;
	wv_modes.mode_gc4 = 3;
	wv_modes.mode_gc8 = 2;
	wv_modes.mode_gc16 = 2;
	wv_modes.mode_gc32 = 2;
	retval = ioctl(fd_fb, MXCFB_SET_WAVEFORM_MODES, &wv_modes);
	if (retval < 0)
	{
		goto err2;
	}

	if (test_updates()) {
		printf("\nError: Updates test failed.\n");
		goto err2;
	}

	if (test_rotation()) {
		printf("\nError: Rotation test failed.\n");
		goto err2;
	}
	if (test_y8()) {
		printf("\nError: Y8 test failed.\n");
		goto err2;
	}

	if (test_auto_waveform()) {
		printf("\nError: Auto waveform selection test failed.\n");
		goto err2;
	}

	if (test_pan()){
		printf("\nError: Pan test failed.\n");
		goto err2;
	}

	if (test_overlay()){
		printf("\nError: Overlay test failed.\n");
		goto err2;
	}

	if (test_auto_update()){
		printf("\nError: Auto update test failed.\n");
		goto err2;
	}

err2:
	munmap(fb, g_fb_size);
err1:
	close(fd_fb);
err0:
	return retval;
}
