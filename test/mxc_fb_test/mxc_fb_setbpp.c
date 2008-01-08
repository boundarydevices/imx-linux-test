/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All rights reserved.
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
 * @file mxc_fb_setbpp.c
 * 
 * @brief Mxc FB test application to set bits per pixel of FB
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
    
/* Verification Test Environment Include Files */
#include <sys/types.h>  
#include <sys/stat.h>   
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>    
#include <stdio.h>
#include <stdlib.h>
#include <asm/types.h>
#include <linux/compiler.h>
#include <linux/fb.h>
#include <math.h>
#include <string.h>
#include <malloc.h>

#define TFAIL -1
#define TPASS 0

int fd_fb = 0;



int 
main(int argc, char **argv)
{
        int bpp;
        int retval = TPASS;
        char fbdev[] = "/dev/fb0";
        struct fb_var_screeninfo screen_info;

        if (argc != 3) {
                printf("Usage:\n\nmxc_fb_setbpp <bpp> <fb #>\n\n");
                return -1;
        }

        bpp = atoi(argv[1]);
        fbdev[7] = argv[2][0];
        
        if ((fd_fb = open(fbdev, O_RDWR, 0)) < 0)
        {
                printf("Unable to open %s\n", fbdev);
                retval = TFAIL;
                goto err0;
        }

        retval = ioctl(fd_fb, FBIOGET_VSCREENINFO, &screen_info);
        if (retval < 0)
        {
                goto err1;
        }
        printf("Changing fb%s bits per pixel from %d-bpp to %d-bpp\n", 
                argv[2], screen_info.bits_per_pixel, bpp);
        screen_info.activate = FB_ACTIVATE_NOW;
        screen_info.bits_per_pixel = bpp;
        retval = ioctl(fd_fb, FBIOPUT_VSCREENINFO, &screen_info);
        if (retval < 0)
        {
                printf("Error setting bpp - %d\n", retval);
                goto err1;
        }

err1:
        close(fd_fb);
err0:
        return retval;
}

