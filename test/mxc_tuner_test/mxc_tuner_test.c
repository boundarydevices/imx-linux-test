/*
 * Copyright (C) 2013-2015 Freescale Semiconductor, Inc. All rights reserved.
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
#define TUNER_SETRSSI_THRESHOLD	4
#define TUNER_SETSNR_THRESHOLD	5
#define TUNER_SETMAX_TUNE_ERROR_THRESHOLD 6
#define TUNER_GETRSSI_SNR_THRESHOLD 7
#define TUNER_SEEKFREQ  8
#define TUNER_OFF	9
#define TUNER_EXIT	10

#define V4L2_CID_USER_SI476X_BASE (V4L2_CID_USER_BASE + 0x1040)
enum si476x_ctrl_id {
	V4L2_CID_SI476X_RSSI_THRESHOLD  = (V4L2_CID_USER_SI476X_BASE + 1),
	V4L2_CID_SI476X_SNR_THRESHOLD   = (V4L2_CID_USER_SI476X_BASE + 2),
	V4L2_CID_SI476X_MAX_TUNE_ERROR  = (V4L2_CID_USER_SI476X_BASE + 3),
	V4L2_CID_SI476X_HARMONICS_COUNT = (V4L2_CID_USER_SI476X_BASE + 4),
	V4L2_CID_SI476X_DIVERSITY_MODE  = (V4L2_CID_USER_SI476X_BASE + 5),
	V4L2_CID_SI476X_INTERCHIP_LINK  = (V4L2_CID_USER_SI476X_BASE + 6),
};

int main(int argc, char **argv)
{
	int fd, ret, n_selection, freq, core;
	struct v4l2_frequency vt;
	struct v4l2_hw_freq_seek seek;
	struct stat buf;
	struct v4l2_control vctl;
	char *dev = NULL;

	/* Check if using si476x drivers, return 0 means true */
	core = stat("/sys/bus/i2c/drivers/si476x-core", &buf);
	printf("Using si476%c drivers.....\n", core ? '3' : 'x');

	memset(&vt, 0, sizeof(struct v4l2_frequency));
	memset(&seek, 0, sizeof(struct v4l2_hw_freq_seek));
	do{
		printf("Welcome to radio menu\n");
		printf("1.-Turn on the radio\n");
		printf("2.-Get current frequency\n");
		printf("3.-Set current frequency\n");
		printf("4.-Set current rssi threshold\n");
		printf("5.-Set current snr threshold\n");
		printf("6.-Set current max tuner error threshold\n");
		printf("7.-get current rssi and snr threshold\n");;
		printf("8.-seek frequcncy\n");
		printf("9.-Turn off the radio\n");
		printf("10.-Exit\n");
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
		case TUNER_SETRSSI_THRESHOLD:
			vctl.id = V4L2_CID_SI476X_RSSI_THRESHOLD;
			printf("Set rssi threshold = ");
			if (scanf("%d", &vctl.value) == 1) {
				ret = ioctl(fd, VIDIOC_S_CTRL, &vctl);
				if (ret < 0) {
					perror("error: Turn on the radio first\n");
					break;
				}
			}
			break;
		case TUNER_SETSNR_THRESHOLD:
			vctl.id = V4L2_CID_SI476X_SNR_THRESHOLD;
			printf("Set snr threshold = ");
			if (scanf("%d", &vctl.value) == 1) {
				ret = ioctl(fd, VIDIOC_S_CTRL, &vctl);
				if (ret < 0) {
					perror("error: Turn on the radio first\n");
					break;
				}
			}
			break;
		case TUNER_SETMAX_TUNE_ERROR_THRESHOLD:
			vctl.id = V4L2_CID_SI476X_MAX_TUNE_ERROR;
			printf("Set max_tune_error threshold = ");
			if (scanf("%d", &vctl.value) == 1) {
				ret = ioctl(fd, VIDIOC_S_CTRL, &vctl);
				if (ret < 0) {
					perror("error: Turn on the radio first\n");
					break;
				}
			}
			break;
		case TUNER_GETRSSI_SNR_THRESHOLD:
			vctl.id = V4L2_CID_SI476X_RSSI_THRESHOLD;
			vctl.value = 0;
			ret = ioctl(fd, VIDIOC_G_CTRL, &vctl);
			if (ret < 0) {
				perror("error: Turn on the radio first\n");
				break;
			}
			printf("rssi threshold = %d", vctl.value);

			vctl.id = V4L2_CID_SI476X_SNR_THRESHOLD;
			vctl.value = 0;
			ret = ioctl(fd, VIDIOC_G_CTRL, &vctl);
			if (ret < 0) {
				perror("error: Turn on the radio first\n");
				break;
			}
			printf(", snr threshold = %d\n", vctl.value);

			break;
		case TUNER_SEEKFREQ:
			/*
			 * v4l2_hw_freq_seek
			 * seek_upward = 1(0): seek upward(downward)
			 * wrap_around = 1(0): warp around(stop) when reach the high range
			 * rangehigh : high value of the seek range
			 * rangelow: low value of the seek range
			 * spacing: searching interval
			 */
			seek.tuner = 0;
			seek.seek_upward = 1;
			seek.wrap_around = 1;
			seek.type = V4L2_TUNER_RADIO;

			ret = ioctl(fd, VIDIOC_S_HW_FREQ_SEEK, &seek);
			if (ret < 0) {
				printf("error: failed seek freq\n");
				break;
			};

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
