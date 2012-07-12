/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#define TRIES		25		/* get 25 samples                 */
#define LOCKTIME	400000		/* wait 400ms for card to lock on */
#define SAMPLEDELAY	15000		/* wait 15ms between samples      */
#define THRESHOLD	.5		/* minimum acceptable signal %    */

int main(int argc, char **argv)
{
	int	fd, ret;
	struct	v4l2_frequency vt;
	int n_selection, freq;
	char *dev = NULL;

	do{
		printf("Welcome to radio menu\n");
		printf("1.-Turn on the radio\n");
		printf("2.-Get current frequency\n");
                printf("3.-Set current frequency\n");
                printf("4.-Turn off the radio\n");
		printf("9.-Exit\n");
		printf("Enter selection number: ");
		if(scanf("%d", &n_selection) == 1){
			switch(n_selection){
			    case 1:
				dev = strdup("/dev/radio0");    /* default */
				fd = open(dev, O_RDONLY);
        			if (fd < 0) {
		                    fprintf(stderr, "Unable to open %s: %s\n", dev, strerror(errno));
    			            exit(1);
        			}
			    break;
                            case 2:
                                ret = ioctl(fd, VIDIOC_G_FREQUENCY, &vt);   /* get initial info */
			        if (ret < 0) {
		                    printf("error: Turn on the radio first\n");
                		    break;
        			}
        			printf("Frequency = %d\n", vt.frequency);
                            break;

                            case 3:
				ret = ioctl(fd, VIDIOC_G_FREQUENCY, &vt);   /* get initial info */
                                if (ret < 0) {
                                    printf("error: Turn on the radio first\n");
                                    break;
                                }
			        printf("Set frequency = ");
				if(scanf("%d", &freq) == 1){
				    vt.frequency = freq;
				    ret = ioctl(fd, VIDIOC_S_FREQUENCY, &vt);   // get initial info
        			    if (ret < 0) {
                                        perror("error: Turn on the radio first\n");
                                        break;
			            }
				}
                            break;
                            case 4:
				if(fd)
                                    close(fd);
                            break;
			    default:
			    break;
			}
		}
	}while(n_selection != 9);

	close(fd);
	return 0;
}
