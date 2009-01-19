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

/*
 * @file mxc_tvout_test.c
 *
 * @brief MXC TVOUT test application
 *
 */

#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/video_encoder.h>

/*
 * FIXME: VGA mode is not defined by video_encoder.h
 * while FS453 supports VGA output.
 */
#ifndef VIDEO_ENCODER_VGA
#define VIDEO_ENCODER_VGA       32
#endif

#define TVOUT_DEV		"/dev/fb/0"
#define MODE_PAL(flags)		(flags & VIDEO_ENCODER_PAL) ? "PAL " : ""
#define MODE_NTSC(flags)	(flags & VIDEO_ENCODER_NTSC) ? "NTSC " : ""
#define MODE_VGA(flags)		(flags & VIDEO_ENCODER_VGA) ? "VGA " : ""

struct video_encoder_capability cap = {0, -1, -1};
static int mode = -1;
static int input = -1;
static int output = -1;
static int enable = -1;

#define MSGHEADER	"tvouttest: "

void usage(char *app)
{
	printf("TVOUT test program.\n");
	printf("Usage: mxc_tvout_test [-h] [-c] [-m mode] [-i input] [-o output] [-e enable]\n");
	printf("\t-h\t  Print this message\n");
	printf("\t-c\t  Get capability\n");
	printf("\t-m\t  Output mode: n=NTSC p=PAL v=VGA\n");
	printf("\t-i\t  Set input\n");
	printf("\t-o\t  Set output\n");
	printf("\t-e\t  Enablie/disable output. y=enable n=disable\n");
	printf("Example:\n");
	printf("%s -c -m p\n", app);
	printf("%s -e y\n", app);
}

int main(int argc, char **argv)
{
	int fd, err, rt;

	if (argc < 2) {
		usage(argv[0]);
		return 0;
	}

	while ((rt = getopt(argc, argv, "hcm:i:o:e:")) >= 0) {
		switch (rt) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			cap.inputs = 0;
			break;
		case 'm':	/* which TV output mode shalll we use */
			if (strcmp(optarg, "p") == 0)
				mode = VIDEO_ENCODER_PAL;
			else if (strcmp(optarg, "n") == 0)
				mode = VIDEO_ENCODER_NTSC;
			else if (strcmp(optarg, "v") == 0)
				mode = VIDEO_ENCODER_VGA;
			else {
				usage(argv[0]);
				return 0;
			}
			break;
		case 'i':
			input = atoi(optarg);
			break;
		case 'o':
			output = atoi(optarg);
			break;
		case 'e':
			if (strcmp(optarg, "y") == 0)
				enable = 1;
			else if (strcmp(optarg, "n") == 0)
				enable = 0;
			else {
				usage(argv[0]);
				return 0;
			}
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	fd = open(TVOUT_DEV, O_RDWR);
	if (fd < 0) {
		printf(MSGHEADER"Can not open %s.\n", TVOUT_DEV);
		return -1;
	}

	if (cap.inputs != -1) {
		err = ioctl(fd, ENCODER_GET_CAPABILITIES, &cap);
		if (err) {
			printf(MSGHEADER"can not get capabilities.\n");
			close(fd);
			return err;
		} else {
			printf("\n"MSGHEADER"Video encoder capabilities:\n"
				"\tflags:\t\t%s%s%s\n"
				"\tinputs:\t\t%d\n"
				"\toutputs:\t%d\n",
				MODE_PAL(cap.flags), MODE_NTSC(cap.flags), MODE_VGA(cap.flags),
				cap.inputs, cap.outputs);
		}
	}

	if (mode != -1) {
		err = ioctl(fd, ENCODER_SET_NORM, &mode);
		if (err) {
			printf(MSGHEADER"can not set output mode.\n");
			close(fd);
			return err;
		}
	}

	if (input != -1) {
		err = ioctl(fd, ENCODER_SET_INPUT, &input);
		if (err) {
			printf(MSGHEADER"can not set input.\n");
			close(fd);
			return err;
		}
	}

	if (output != -1) {
		err = ioctl(fd, ENCODER_SET_OUTPUT, &output);
		if (err) {
			printf(MSGHEADER"can not set output.\n");
			close(fd);
			return err;
		}
	}

	if (enable != -1) {
		err = ioctl(fd, ENCODER_ENABLE_OUTPUT, &enable);
		if (err) {
			printf(MSGHEADER"can not enable/disable output.\n");
			close(fd);
			return err;
		}
	}

	close(fd);

	return 0;
}

