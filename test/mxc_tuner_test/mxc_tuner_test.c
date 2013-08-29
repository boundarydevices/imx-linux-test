/*
 * Copyright (C) 2013 Freescale Semiconductor, Inc. All rights reserved.
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
#include <sys/stat.h>
#include <linux/videodev2.h>

#define TRIES		25		/* Get 25 samples */
#define LOCKTIME	400000		/* Wait 400ms for card to lock on */
#define SAMPLEDELAY	15000		/* Wait 15ms between samples */
#define THRESHOLD	.5		/* Minimum acceptable signal % */

#define TUNER_ON	1
#define TUNER_GETFREQ	2
#define TUNER_SETFREQ	3
#define TUNER_OFF	4
#define TUNER_EXIT	9

int main(int argc, char **argv)
{
	int fd, ret, n_selection, freq, core;
	struct v4l2_frequency vt;
	struct stat buf;
	char *dev = NULL;

	/* Check if using si476x drivers, return 0 means true */
	core = stat("/sys/bus/i2c/drivers/si476x-core", &buf);
	printf("Using si476%c drivers.....\n", core ? '3' : 'x');

	memset(&vt, 0, sizeof(struct v4l2_frequency));

	do{
		printf("Welcome to radio menu\n");
		printf("1.-Turn on the radio\n");
		printf("2.-Get current frequency\n");
		printf("3.-Set current frequency\n");
		printf("4.-Turn off the radio\n");
		printf("9.-Exit\n");
		printf("Enter selection number: ");

		if(scanf("%d", &n_selection) != 1)
			continue;

		switch (n_selection) {
		case TUNER_ON:
			dev = strdup("/dev/radio0");
			fd = open(dev, O_RDONLY);
			if (fd < 0) {
				fprintf(stderr, "Unable to open %s: %s\n", dev, strerror(errno));
				exit(1);
			}
			break;
		case TUNER_GETFREQ:
			ret = ioctl(fd, VIDIOC_G_FREQUENCY, &vt);
			if (ret < 0) {
				printf("error: Turn on the radio first\n");
				break;
			}
			if (!core)
				printf("Frequency = %d\n", vt.frequency * 625 / 100000);
			else
				printf("Frequency = %d\n", vt.frequency);
			break;

		case TUNER_SETFREQ:
			ret = ioctl(fd, VIDIOC_G_FREQUENCY, &vt);
			if (ret < 0) {
				printf("error: Turn on the radio first\n");
				break;
			}
			printf("Set frequency = ");
			if (scanf("%d", &freq) == 1){
				if (!core)
					vt.frequency = freq * 100000 / 625;
				else
					vt.frequency = freq;
				ret = ioctl(fd, VIDIOC_S_FREQUENCY, &vt);
				if (ret < 0) {
					perror("error: Turn on the radio first\n");
					break;
				}
			}
			break;
		case TUNER_OFF:
			if (fd)
				close(fd);
			break;
		default:
			break;
		}
	}while (n_selection != TUNER_EXIT);

	close(fd);
	return 0;
}
