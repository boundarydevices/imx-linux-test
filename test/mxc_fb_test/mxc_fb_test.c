/*
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All rights reserved.
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
 * @file mxc_v4l2_overlay.c
 *
 * @brief Mxc Video For Linux 2 driver test application
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
#include <asm/arch/mxcfb.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>

#define TFAIL -1
#define TPASS 0
#ifdef CONFIG_SDC /* SDC */
#define MXCFB_SCREEN_WIDTH      240
#define MXCFB_SCREEN_HEIGHT     320
#endif
#ifdef CONFIG_ADC /* ADC */
#define MXCFB_SCREEN_WIDTH      176
#define MXCFB_SCREEN_HEIGHT     220
#endif

int fd_fb0 = 0;
int fd_fb1 = 0;
unsigned short * fb0;
unsigned short * fb1;
int g_fb0_size;
int g_fb1_size;
int g_num_buffers;

int g_in_width = 0;
int g_in_height = 0;
int g_display_width = 176;
int g_display_height = 144;
int g_rotate = 0;
int g_bmp_header = 0;
int g_loop_count = 1;

#define MAX_BUFFER_NUM 5

struct testbuffer
{
        unsigned char *start;
        size_t offset;
        unsigned int length;
};

struct testbuffer buffers[5];

#if 0
int process_cmdline(int argc, char **argv)
{
        int i;
        
        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-iw") == 0) {
                        g_in_width = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-ih") == 0) {
                        g_in_height = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-ow") == 0) {
                        g_display_width = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-oh") == 0) {
                        g_display_height = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-r") == 0) {
                        g_rotate = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-l") == 0) {
                        g_loop_count = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-f") == 0) {
                        i++;
                        g_in_fmt = v4l2_fourcc(argv[i][0], argv[i][1],argv[i][2],argv[i][3]);

                        if ( (g_in_fmt != V4L2_PIX_FMT_BGR24) &&
                             (g_in_fmt != V4L2_PIX_FMT_BGR32) &&
                             (g_in_fmt != V4L2_PIX_FMT_RGB565) &&
                             (g_in_fmt != 'PMBW') &&
                             (g_in_fmt != V4L2_PIX_FMT_YUV420) )
                        {
                                return -1;
                        }
                        if (g_in_fmt == 'PMBW') {
                                g_bmp_header = 1;
                                g_in_fmt = V4L2_PIX_FMT_BGR24;
                        }
                }
        }

        printf("g_in_width = %d, g_in_height = %d\n", g_in_width, g_in_height);
        printf("g_display_width = %d, g_display_height = %d\n", g_display_width, g_display_height);
        
        if ((g_in_width == 0) || (g_display_width == 0) || (g_in_height == 0) ||
            (g_display_height == 0)) {
                return -1;
        }
        return 0;
}
#endif

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

        for (y = 0; y < 320*2; y++) {
                for (x = 0; x < 240; x++) {
                        fb[(y*240) + x] = color;
                }
                color+=4;
        }

        for (y = 0; y <= 320; y++) {
                var->yoffset = y;
                retval = ioctl(fd_fb, FBIOPAN_DISPLAY, var);
                if (retval < 0)
                        break;
        }
        printf("Pan test done.\n");
}

int
main(int argc, char **argv)
{
        int retval = TPASS;
        struct mxcfb_gbl_alpha gbl_alpha;
        struct fb_var_screeninfo screen_info;
        u_int32_t screensize = 0;

/*
        if (process_cmdline(argc, argv) < 0) {
                printf("MXC Video4Linux Output Device Test\n\n" \
                       "Syntax: mxc_v4l2_output.out -iw <input width> -ih <input height>\n" \
                       "-ow <display width>\n -oh <display height> -r <rotate mode>\n" \
                       "<input YUV file>\n\n");
                retval = TFAIL;
                goto err0;
        }
*/
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

