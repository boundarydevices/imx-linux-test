/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*=======================================================================
                                        INCLUDE FILES
=======================================================================*/
/* Standard Include Files */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

/* Verification Test Environment Include Files */
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/compiler.h>
#include <math.h>
#include <string.h>

#include <linux/mxcfb.h>


int fd_fb = 0;



int
main(int argc, char **argv)
{
        int count;
	int i;

        int retval = 0;
	unsigned int fps;
	unsigned int total_time;
        char fbdev[] = "/dev/fb0";
        struct timeval tv_start, tv_current;

        if (argc != 3) {
                printf("Usage:\n\n%s <fb #> <count>\n\n", argv[0]);
                return -1;
        }

        count = atoi(argv[2]);
        fbdev[7] = argv[1][0];

        if ((fd_fb = open(fbdev, O_RDWR, 0)) < 0)
        {
                printf("Unable to open %s\n", fbdev);
                retval = -1;
                goto err0;
        }

	gettimeofday(&tv_start, 0);
	for (i = 0; i < count; i++) {
	        retval = ioctl(fd_fb, MXCFB_WAIT_FOR_VSYNC, &i);
        	if (retval < 0)
        	{
                	goto err1;
        	}
	}
	gettimeofday(&tv_current, 0);

        total_time = (tv_current.tv_sec - tv_start.tv_sec) * 1000000L;
        total_time += tv_current.tv_usec - tv_start.tv_usec;
	fps = (i * 1000000ULL) / total_time;
        printf("total time for %u frames = %u us =  %lld fps\n", i, total_time, (i * 1000000ULL) / total_time);

	if (fps > 45 && fps < 80)
		retval = 0;
	else
		retval = -1;

err1:
        close(fd_fb);
err0:
        return retval;
}

