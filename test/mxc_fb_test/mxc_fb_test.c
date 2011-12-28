/*
 * Copyright 2004-2011 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * @file mxc_fb_test.c
 *
 * @brief Mxc framebuffer driver test application
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

#define TFAIL -1
#define TPASS 0

int fd_fb0 = 0;
int fd_fb1 = 0;
unsigned short * fb0;
unsigned short * fb1;
int g_fb0_size;
int g_fb1_size;

void fb_test_bpp(int fd, unsigned short * fb)
{
        int i;
        __u32 screensize;
        int retval;
        struct fb_var_screeninfo screen_info;

        retval = ioctl(fd, FBIOGET_VSCREENINFO, &screen_info);
        if (retval < 0)
        {
                return;
        }

        printf("Set the background to 32-bpp\n");
        screen_info.bits_per_pixel = 32;
        retval = ioctl(fd, FBIOPUT_VSCREENINFO, &screen_info);
        if (retval < 0)
        {
                return;
        }
        printf("fb_test: xres_virtual = %d\n", screen_info.xres_virtual);
        screensize = screen_info.xres * screen_info.yres * screen_info.bits_per_pixel / 8;

        printf("Fill the BG in red, size = 0x%08X\n", screensize);
        for (i = 0; i < screensize/4; i++)
                ((__u32*)fb)[i] = 0x00FF0000;
        sleep(3);

        printf("Set the BG to 24-bpp\n");
        screen_info.bits_per_pixel = 24;
        retval = ioctl(fd, FBIOPUT_VSCREENINFO, &screen_info);
        if (retval < 0)
        {
                return;
        }
        printf("fb_test: xres_virtual = %d\n", screen_info.xres_virtual);
        screensize = screen_info.xres * screen_info.yres * screen_info.bits_per_pixel / 8;

        printf("Fill the BG in blue, size = 0x%08X\n", screensize);
        for (i = 0; i < screensize; ) {
                ((__u8*)fb)[i++] = 0xFF;       // Blue
                ((__u8*)fb)[i++] = 0x00;       // Green
                ((__u8*)fb)[i++] = 0x00;       // Red
        }
        sleep(3);

        printf("Set the background to 16-bpp\n");
        screen_info.bits_per_pixel = 16;
        retval = ioctl(fd, FBIOPUT_VSCREENINFO, &screen_info);
        if (retval < 0)
        {
                return;
        }
        screensize = screen_info.xres * screen_info.yres * screen_info.bits_per_pixel / 8;

        printf("Fill the BG in green, size = 0x%08X\n", screensize);
        for (i = 0; i < screensize/2; i++)
                fb[i] = 0x07E0;
        sleep(3);

}

void fb_test_gbl_alpha(void)
{
        int i;
        int retval;
        volatile int delay;
        struct mxcfb_gbl_alpha gbl_alpha;
        struct mxcfb_color_key key;

        printf("Testing global alpha blending...\n");

        printf("Fill the FG in black \n");
        memset(fb1, 0x0, g_fb1_size);
        sleep(2);

        printf("Fill the BG in white \n");
        memset(fb0, 0xFF, g_fb0_size);
        sleep(2);

        gbl_alpha.enable = 1;
        for (i = 0; i < 0x100; i++) {
                delay = 1000;

                gbl_alpha.alpha = i;
                retval = ioctl(fd_fb0, MXCFB_SET_GBL_ALPHA, &gbl_alpha);

                // Wait for VSYNC
                retval = ioctl(fd_fb0, MXCFB_WAIT_FOR_VSYNC, 0);
                if (retval < 0) {
                        printf("Error waiting on VSYNC\n");
                        break;
                }
        }

        for (i = 0xFF; i > 0; i--) {
                delay = 1000;

                gbl_alpha.alpha = i;
                retval = ioctl(fd_fb0, MXCFB_SET_GBL_ALPHA, &gbl_alpha);

                while (delay--) ;
        }
        printf("Alpha is 0, FG is opaque\n");
        sleep(3);

        gbl_alpha.alpha = 0xFF;
        retval = ioctl(fd_fb0, MXCFB_SET_GBL_ALPHA, &gbl_alpha);
        printf("Alpha is 255, BG is opaque\n");
        sleep(3);

        key.enable = 1;
        key.color_key = 0x00FF0000; // Red
        retval = ioctl(fd_fb0, MXCFB_SET_CLR_KEY, &key);

        for (i = 0; i < 240*80; i++)
                fb0[i] = 0xF800;

        printf("Color key enabled\n");
        sleep(3);

        key.enable = 0;
        retval = ioctl(fd_fb0, MXCFB_SET_CLR_KEY, &key);
        printf("Color key disabled\n");


        gbl_alpha.enable = 0;
        retval = ioctl(fd_fb0, MXCFB_SET_GBL_ALPHA, &gbl_alpha);
        printf("Global alpha disabled\n");
        sleep(3);

}

void fb_test_pan(int fd_fb, unsigned short * fb, struct fb_var_screeninfo * var)
{
        int x, y;
        int retval;
        int color = 0;

        printf("Pan test start.\n");

        for (y = 0; y < var->yres_virtual; y++) {
                for (x = 0; x < var->xres; x++) {
                        fb[(y*var->xres) + x] = color;
                }
                color+=4;
        }

        for (y = 0; y <= var->yres; y++) {
                var->yoffset = y;
                retval = ioctl(fd_fb, FBIOPAN_DISPLAY, var);
                if (retval < 0)
                        break;
        }
        printf("Pan test done.\n");
}

void get_gamma_coeff(float gamma, int constk[], int slopek[])
{
	unsigned int tk[17] = {0, 2, 4, 8, 16, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 383};
        unsigned int k;
        unsigned int rk[16], yk[17];
        float center, width, start, height, t, scurve[17], gammacurve[17];

        center = 32; /* nominal 32 - must be in {1,380} where is s-curve centered*/
        width = 32; /* nominal 32 - must be in {1,380} how narrow is s-curve */
        for(k=0;k<17;k++) {
                t = (float)tk[k];
                scurve[k] = (256.F/3.14159F)*((float) atan((t-center)/width));
                gammacurve[k] = 255.F* ((float) pow(t/384.F,1.0/gamma));
        }

        start = scurve[0];
        height = scurve[16] - start;
        for(k=0;k<17;k++) {
                scurve[k] = (256.F/height)*(scurve[k]-start);
                yk[k] = (int)(scurve[k] * gammacurve[k]/256.F + 0.5) << 1;
        }

        for(k=0;k<16;k++)
                rk[k] = yk[k+1] - yk[k];

        for(k=0;k<5;k++) {
                constk[k] = yk[k] - rk[k];
                slopek[k] = rk[k] << (4-k);
        }
        for(k=5;k<16;k++) {
                constk[k] = yk[k];
                slopek[k] = rk[k] >> 1;
        }

	for(k=0;k<16;k++) {
		constk[k] /= 2;
		constk[k] &= 0x1ff;
	}
}

void fb_test_gamma(int fd_fb)
{
	struct mxcfb_gamma gamma;
	float gamma_tc[5] = {0.8, 1.0, 1.5, 2.2, 2.4};
	int i, retval;

        printf("Gamma test start.\n");
	for (i=0;i<5;i++) {
		printf("Gamma %f\n", gamma_tc[i]);
		get_gamma_coeff(gamma_tc[i], gamma.constk, gamma.slopek);
		gamma.enable = 1;
		retval = ioctl(fd_fb, MXCFB_SET_GAMMA, &gamma);
		if (retval < 0) {
			printf("Ioctl MXCFB_SET_GAMMA fail!\n");
			break;
		}
		sleep(3);
	}

        printf("Gamma test end.\n");
}

int
main(int argc, char **argv)
{
        int retval = TPASS;
        struct mxcfb_gbl_alpha gbl_alpha;
        struct fb_var_screeninfo screen_info;
        struct fb_fix_screeninfo fb_fix;
        u_int32_t screensize = 0;

        if ((fd_fb0 = open("/dev/fb0", O_RDWR, 0)) < 0)
        {
                printf("Unable to open /dev/fb0\n");
                retval = TFAIL;
                goto err0;
        }

        if ((fd_fb1 = open("/dev/fb1", O_RDWR, 0)) < 0)
        {
                printf("Unable to open /dev/fb1\n");
                retval = TFAIL;
                goto err0;
        }
	retval = ioctl(fd_fb1, FBIOGET_FSCREENINFO, &fb_fix);
        if (retval < 0)
        {
                goto err1;
        }
        retval = ioctl(fd_fb1, FBIOBLANK, FB_BLANK_UNBLANK);
        if (retval < 0)
        {
                goto err1;
        }

        printf("Opened fb0 and fb1\n");

        retval = ioctl(fd_fb0, FBIOGET_VSCREENINFO, &screen_info);
        if (retval < 0)
        {
                goto err2;
        }
        printf("Set the background to 16-bpp\n");
        screen_info.bits_per_pixel = 16;
        screen_info.yoffset = 0;
        retval = ioctl(fd_fb0, FBIOPUT_VSCREENINFO, &screen_info);
        if (retval < 0)
        {
                goto err2;
        }
        g_fb0_size = screen_info.xres * screen_info.yres_virtual * screen_info.bits_per_pixel / 8;

        printf("\n Screen size = %u \n", screensize);

        /* Map the device to memory*/
        fb0 = (unsigned short *)mmap(0, g_fb0_size,PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb0, 0);
        if ((int)fb0 <= 0)
        {
                printf("\nError: failed to map framebuffer device 0 to memory.\n");
                goto err2;
        }

        retval = ioctl(fd_fb1, FBIOGET_VSCREENINFO, &screen_info);
        if (retval < 0)
        {
                goto err3;
        }
        printf("Set the foreground to 16-bpp\n");
        screen_info.bits_per_pixel = 16;
        screen_info.yoffset = 0;
        retval = ioctl(fd_fb1, FBIOPUT_VSCREENINFO, &screen_info);
        if (retval < 0)
        {
                goto err3;
        }
        g_fb1_size = screen_info.xres * screen_info.yres_virtual * screen_info.bits_per_pixel / 8;

        /* Map the device to memory*/
        fb1 = (unsigned short *)mmap(0, g_fb1_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb1, 0);
        if ((int)fb1 <= 0)
        {
                printf("\nError: failed to map framebuffer device 1 to memory.\n");
                goto err3;
        }

        fb_test_gbl_alpha();

        retval = ioctl(fd_fb1, FBIOGET_VSCREENINFO, &screen_info);
        if (retval < 0) {
                goto err4;
        }
        printf("Set the background to 16-bpp\n");
        screen_info.bits_per_pixel = 16;
        retval = ioctl(fd_fb1, FBIOPUT_VSCREENINFO, &screen_info);
        if (retval < 0) {
                goto err4;
        }
        fb_test_pan(fd_fb1, fb1, &screen_info);

        // Set BG to visable
        gbl_alpha.alpha = 0xFF;
        retval = ioctl(fd_fb0, MXCFB_SET_GBL_ALPHA, &gbl_alpha);

        retval = ioctl(fd_fb0, FBIOGET_VSCREENINFO, &screen_info);
        if (retval < 0) {
                goto err4;
        }
        printf("Set the background to 16-bpp\n");
        screen_info.bits_per_pixel = 16;
        retval = ioctl(fd_fb0, FBIOPUT_VSCREENINFO, &screen_info);
        if (retval < 0) {
                goto err4;
        }
        fb_test_pan(fd_fb0, fb0, &screen_info);

        fb_test_gamma(fd_fb0);
err4:
        munmap(fb1, g_fb1_size);
err3:
        munmap(fb0, g_fb0_size);
err2:
        close(fd_fb1);
err1:
        close(fd_fb0);
err0:
        return retval;
}

