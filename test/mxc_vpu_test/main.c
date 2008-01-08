
/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Copyright (c) 2006, Chips & Media.  All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include "vpu_test.h"

char *usage = "Usage: ./mxc_vpu_test.out -D \"<decode options>\" "\
	       "-E \"<encode options>\" "\
	       "-L \"<loopback options>\" -C <config file> "\
	       "-H display this help \n "
	       "\n"\
	       "decode options \n "\
	       "  -i <input file> Read input from file \n "\
	       "	If no input file is specified, default is network \n "\
	       "  -o <output file> Write output to file \n "\
	       "	If no output is specified, default is LCD \n "\
	       "  -f <format> 0 - MPEG4, 1 - H.263, 2 - H.264, 3 - VC1 \n "\
	       "	If no format specified, default is 0 (MPEG4) \n "\
	       "  -p <port number> UDP port number to bind \n "\
	       "	If no port number is secified, 5555 is used \n "\
	       "  -c <count> Number of frames to decode \n "\
	       "  -d <deblocking> Enable deblock - 1. Not for MX27 \n "\
	       "	default deblock is disabled (0). \n "\
	       "  -r <rotation angle> 0, 90, 180, 270 \n "\
	       "	default rotation is disabled (0) \n "\
	       "  -m <mirror direction> 0, 1, 2, 3 \n "\
	       "	default no mirroring (0) \n "\
	       "\n"\
	       "encode options \n "\
	       "  -i <input file> Read input from file (yuv) \n "\
	       "	If no input file specified, default is camera \n "\
	       "  -o <output file> Write output to file \n "\
	       "	This option will be ignored if 'n' is specified \n "\
	       "	If no output is specified, def files are created \n "\
	       "  -n <ip address> Send output to this IP address \n "\
	       "  -p <port number> UDP port number at server \n "\
	       "	If no port number is secified, 5555 is used \n "\
	       "  -f <format> 0 - MPEG4, 1 - H.263, 2 - H.264, 3 - VC1 \n "\
	       "	If no format specified, default is 0 (MPEG4) \n "\
	       "  -c <count> Number of frames to encode \n "\
	       "  -r <rotation angle> 0, 90, 180, 270 \n "\
	       "	default rotation is disabled (0) \n "\
	       "  -m <mirror direction> 0, 1, 2, 3 \n "\
	       "	default no mirroring (0) \n "\
	       "  -w <width> capture image width \n "\
	       "	default is 176. \n "\
	       "  -h <height>capture image height \n "\
	       "	default is 144 \n "\
	       "  -b <bitrate in kbps> \n "\
	       "	default is auto (0) \n "\
	       "  -g <gop size> \n "\
	       "	default is 0 \n"
	       "\n"\
	       "loopback options \n "\
	       "  -f <format> 0 - MPEG4, 1 - H.263, 2 - H.264, 3 - VC1 \n "\
	       "	If no format specified, default is 0 (MPEG4) \n "\
	       "  -w <width> capture image width \n "\
	       "	default is 176. \n "\
	       "  -h <height>capture image height \n "\
	       "	default is 144 \n "\
	       "\n"\
	       "config file - Use config file for specifying options \n";

struct input_argument {
	int mode;
	pthread_t tid;
	char line[128];
	struct cmd_line cmd;
};

sigset_t sigset;
int quitflag;

static struct input_argument input_arg[4];
static int instance;
static int using_config_file;
static pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;

int decode_test(void *arg);
int encode_test(void *arg);
int encdec_test(void *arg);

/* Encode or Decode or Loopback */
static char *mainopts = "HE:D:L:C:";

/* Options for encode and decode */
static char *options = "i:o:n:p:r:f:c:w:h:g:b:d:m:";

int 
parse_config_file(char *file_name)
{
	FILE *fp;
	char line[128];
	char *ptr;
	int end;

	fp = fopen(file_name, "r");
	if (fp == NULL) {
		printf("Failed to open config file\n");
		return -1;
	}

	while (fgets(line, 64, fp) != NULL) {
		if (instance > 3) {
			printf("No more instances!!\n");
			break;
		}

		ptr = skip_unwanted(line);

		end = parse_options(ptr, &input_arg[instance].cmd, 
					&input_arg[instance].mode);
		if (end == 100) {
			instance++;
		}
	}

	fclose(fp);
	return 0;
}

int 
parse_main_args(int argc, char *argv[])
{
	int status = 0, opt;

	do {
		opt = getopt(argc, argv, mainopts);
		switch (opt)
		{
		case 'D':
			input_arg[instance].mode = DECODE;
			strncpy(input_arg[instance].line, argv[0], 26);
			strncat(input_arg[instance].line, " ", 2);
			strncat(input_arg[instance].line, optarg, 100);
			instance++;
			break;
		case 'E':
			input_arg[instance].mode = ENCODE;
			strncpy(input_arg[instance].line, argv[0], 26);
			strncat(input_arg[instance].line, " ", 2);
			strncat(input_arg[instance].line, optarg, 100);
			instance++;
			break;
		case 'L':
			input_arg[instance].mode = LOOPBACK;
			strncpy(input_arg[instance].line, argv[0], 26);
			strncat(input_arg[instance].line, " ", 2);
			strncat(input_arg[instance].line, optarg, 100);
			instance++;
			break;
		case 'C':
			if (instance > 0) {
			 	printf("-C option not selected because of other options\n");
				break;
			}

			if (parse_config_file(optarg) == 0) {
				using_config_file = 1;
			}

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

int 
parse_args(int argc, char *argv[], int i)
{
	int status = 0, opt;
	
	do {
		opt = getopt(argc, argv, options);
		switch (opt)
		{
		case 'i':
			strncpy(input_arg[i].cmd.input, optarg, 64);
			input_arg[i].cmd.src_scheme = PATH_FILE;
			break;
		case 'o':
			if (input_arg[i].cmd.dst_scheme == PATH_NET) {
				printf("warn: -o ignored because of -n\n");
				break;
			}
			strncpy(input_arg[i].cmd.output, optarg, 64);
			input_arg[i].cmd.dst_scheme = PATH_FILE; 
			break;
		case 'n':
			if (input_arg[i].mode == ENCODE) {
				/* contains the ip address */
				strncpy(input_arg[i].cmd.output, optarg, 64);
				input_arg[i].cmd.dst_scheme = PATH_NET; 
			} else {
				printf("Warn:-n option used only for encode\n");
			}
			break;
		case 'p':
			input_arg[i].cmd.port = atoi(optarg);
			break;
		case 'r':
			input_arg[i].cmd.rot_en = 1;
			input_arg[i].cmd.rot_angle = atoi(optarg);
			break;
		case 'f':
			input_arg[i].cmd.format = atoi(optarg);
			break;
		case 'c':
			input_arg[i].cmd.count = atoi(optarg);
			break;
		case 'w':
			input_arg[i].cmd.width = atoi(optarg);
			break;
		case 'h':
			input_arg[i].cmd.height = atoi(optarg);
			break;
		case 'g':
			input_arg[i].cmd.gop = atoi(optarg);
			break;
		case 'b':
			input_arg[i].cmd.bitrate = atoi(optarg);
			break;
		case 'd':
			input_arg[i].cmd.mp4dblk_en = 1; 
			break;
		case 'm':
			input_arg[i].cmd.mirror = atoi(optarg); 
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

static int 
signal_thread(void *arg)
{
	int sig, err;

	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	while (1) {
		err = sigwait(&sigset, &sig);
		if (sig == SIGINT) {
			printf("Ctrl-C received\n");
		} else {
			printf("Unknown signal. Still exiting\n");
		}
		quitflag = 1;
		break;
	}

	return 0;
}

int 
main(int argc, char *argv[])
{
	int err, nargc, i, ret = 0;
	char *pargv[32] = {0};
	pthread_t sigtid;
	vpu_versioninfo ver;

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

	framebuf_init();

	err = IOSystemInit(NULL);
	if (err) {
		printf("IOSystemInit failure\n");
		return -1;
	}

	err = vpu_GetVersionInfo(&ver);
	if (err) {
		printf("Cannot get version info\n");
		IOSystemShutdown();
		return -1;
	}

	printf("VPU firmware version: %d.%d.%d\n", ver.fw_major, ver.fw_minor,
						ver.fw_release);
	printf("VPU library version: %d.%d.%d\n", ver.lib_major, ver.lib_minor,
						ver.lib_release);

	if (instance > 1) {
		pthread_mutex_init(&fastmutex, NULL);
		
		for (i = 0; i < instance; i++) {
			if (using_config_file == 0) {
				get_arg(input_arg[i].line, &nargc, pargv);
				err = parse_args(nargc, pargv, i);
				if (err) {
					IOSystemShutdown();
					goto usage;
				}
			}

			if (check_params(&input_arg[i].cmd, 
						input_arg[i].mode) == 0) {
				if (open_files(&input_arg[i].cmd) == 0) {
					input_arg[i].cmd.mutex = &fastmutex;
					if (input_arg[i].mode == DECODE) {
					     pthread_create(&input_arg[i].tid,
						   NULL,
						   (void *)&decode_test,
						   (void *)&input_arg[i].cmd);
					} else if (input_arg[i].mode == 
							ENCODE) {
					     pthread_create(&input_arg[i].tid,
						   NULL, 
						   (void *)&encode_test,
						   (void *)&input_arg[i].cmd);
					}
				}
			}
			
		}
	} else {
		if (using_config_file == 0) {
			get_arg(input_arg[0].line, &nargc, pargv);
			err = parse_args(nargc, pargv, 0);
			if (err) {
				IOSystemShutdown();
				goto usage;
			}
		}

		if (check_params(&input_arg[0].cmd, input_arg[0].mode) == 0) {
			if (open_files(&input_arg[0].cmd) == 0) {
				if (input_arg[0].mode == DECODE) {
					ret = decode_test(&input_arg[0].cmd);
				} else if (input_arg[0].mode == ENCODE) {
					ret = encode_test(&input_arg[0].cmd);
				}
			
				close_files(&input_arg[0].cmd);
			} else {
				ret = -1;
			}
		} else {
			ret = -1;
		}
		
		if (input_arg[0].mode == LOOPBACK) {
			encdec_test(&input_arg[0].cmd);
		}
	}

	if (instance > 1) {
		for (i = 0; i < instance; i++) {
			if (input_arg[i].tid != 0) {
				pthread_join(input_arg[i].tid, NULL);
				close_files(&input_arg[i].cmd);
			}
		}

		pthread_mutex_destroy(&fastmutex);
	}

	IOSystemShutdown();
	return ret;

usage:
	printf("\n%s", usage);
	return -1;
}

