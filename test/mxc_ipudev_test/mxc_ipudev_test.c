/*
 * Copyright 2009 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 */

/*
 * The code contained herein is licensed under the GNU Lesser General
 * Public License.  You may obtain a copy of the GNU Lesser General
 * Public License Version 2.1 or later at the following locations:
 *
 * http://www.opensource.org/licenses/lgpl-license.html
 * http://www.gnu.org/copyleft/lgpl.html
 */

/*!
 * @file mxc_ipudev_test.c
 *
 * @brief IPU device lib test implementation
 *
 * @ingroup IPU
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include "mxc_ipudev_test.h"

int ctrl_c_rev = 0;

void ctrl_c_handler(int signum, siginfo_t *info, void *myact)
{
	ctrl_c_rev = 1;
}

void output_to_file_cb(void * arg, int index)
{
	ipu_test_handle_t * test_handle = (ipu_test_handle_t *)arg;

	if (test_handle->file_out0)
		if(fwrite(test_handle->ipu_handle->outbuf_start0[index], 1,
				test_handle->ipu_handle->ofr_size[0],
				test_handle->file_out0) < test_handle->ipu_handle->ofr_size[0]) {
			printf("Can not write enough data into output file!\n");
		}
	if (test_handle->file_out1)
		if(fwrite(test_handle->ipu_handle->outbuf_start1[index], 1,
				test_handle->ipu_handle->ofr_size[1],
				test_handle->file_out1) < test_handle->ipu_handle->ofr_size[1]) {
			printf("Can not write enough data into output file!\n");
		}
}

int process_cmdline(int argc, char **argv, ipu_test_handle_t * test_handle)
{
	int i;

	if (argc == 1)
		return -1;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-C") == 0) {
			parse_config_file(argv[++i], test_handle);
		} else if (strcmp(argv[i], "-P") == 0) {
			test_handle->test_pattern = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-bw") == 0) {
			test_handle->block_width = atoi(argv[++i]);
			if (test_handle->block_width < 16)
				test_handle->block_width = 16;
		}
	}

	if (test_handle->test_pattern)
		return 0;

	if ((test_handle->input.width == 0) || (test_handle->input.height == 0) ||
			(test_handle->output0.width == 0) ||
			(test_handle->output0.height == 0)
			|| (test_handle->fcount < 1))
		return -1;

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0, next_update_idx = 0, done_cnt = 0, first_time = 1;
	ipu_lib_handle_t ipu_handle;
	ipu_test_handle_t test_handle;
	FILE * file_in = NULL;
	struct sigaction act;

	memset(&ipu_handle, 0, sizeof(ipu_lib_handle_t));
	memset(&test_handle, 0, sizeof(ipu_test_handle_t));
	test_handle.ipu_handle = &ipu_handle;
	test_handle.mode = OP_NORMAL_MODE;
	test_handle.block_width = 80;

	/*for ctrl-c*/
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = ctrl_c_handler;

	if((ret = sigaction(SIGINT, &act, NULL)) < 0) {
		printf("install sigal error\n");
		goto done;
	}

	if (process_cmdline(argc, argv, &test_handle) < 0) {
		printf("\nMXC IPU device Test\n\n" \
				"Usage: %s\n" \
				"-C <config file>\n" \
				"-P <test pattern>\n" \
				"[-bw <block width for pattern 3>]\n" \
				"<input raw file>\n\n", argv[0]);
		printf("test pattern:\n" \
			"1: video pattern with user define dma buffer queue, one full-screen output\n" \
			"2: video pattern with user define dma buffer queue, with two output\n" \
			"3: hopping block screen save\n" \
			"4: color bar + hopping block\n" \
			"5: color bar IC global alpha overlay\n" \
			"6: color bar IC separate local alpha overlay\n" \
			"7: color bar IC local alpha within pixel overlay\n" \
			"8: ipu dma copy test\n" \
			"9: 2 screen layer test using IC global alpha blending\n" \
			"10: 3 screen layer test using IC global alpha blending\n" \
			"11: 2 screen layer test using IC local alpha blending with alpha value in separate buffer\n" \
			"12: 3 screen layer test using IC local alpha blending with alpha value in separate buffer\n" \
			"13: 2 screen layer test using IC local alpha blending with alpha value in pixel\n" \
			"14: 3 screen layer test using IC local alpha blending with alpha value in pixel\n" \
			"15: 2 screen layer test IPC ProcessA + ProcessB with globla alpha blending\n" \
			"16: 2 screen layer test IPC ProcessA + ProcessB with local alpha blending\n" \
			"17: 3 screen layer test IPC ProcessA(first_layer + sencond_layer) + ProcessB(third_layer) with globla alpha blending\n" \
			"18: 3 screen layer test IPC ProcessA(first_layer + sencond_layer) + ProcessB(third_layer) with local alpha blending\n" \
			"19: 3 screen layer test IPC ProcessA(first_layer) ProcessB(sencond_layer) ProcessC(third_layer) with local alpha blending\n" \
			"20: 2 screen layer test IPC ProcessA(first_layer) ProcessB(sencond_layer) with DP local alpha blending\n" \
			"21: 2 screen layer test IPC ProcessA(first_layer) ProcessB(sencond_layer) with local alpha blending plus tv copy\n" \
			"22: Horizontally splitted video test on TV(support upsizing), assuming the TV uses MEM_DC_SYNC channel\n\n");
		return -1;
	}

	system("echo 0,0 > /sys/class/graphics/fb0/pan");

	if (test_handle.test_pattern) {
		ret = run_test_pattern(test_handle.test_pattern, &test_handle);
		system("echo 0,0 > /sys/class/graphics/fb0/pan");
		return ret;
	}

	if (test_handle.mode & OP_STREAM_MODE) {
		if (test_handle.fcount == 1) {
			test_handle.mode &= ~(OP_STREAM_MODE);
			test_handle.mode |= OP_NORMAL_MODE;
		}
	}

	file_in = fopen(argv[argc-1], "rb");
	if (file_in == NULL){
		printf("there is no such file for reading %s\n", argv[argc-1]);
		return -1;
	}

	if (test_handle.outfile0 && !test_handle.output0.show_to_fb)
		test_handle.file_out0 = fopen(test_handle.outfile0, "wb");
	if (test_handle.outfile1 && !test_handle.output1.show_to_fb &&
		test_handle.output1_enabled)
		test_handle.file_out1 = fopen(test_handle.outfile1, "wb");

	if (test_handle.output1_enabled)
		ret = mxc_ipu_lib_task_init(&(test_handle.input), NULL, &(test_handle.output0),
				&(test_handle.output1), test_handle.mode, test_handle.ipu_handle);
	else
		ret = mxc_ipu_lib_task_init(&(test_handle.input), NULL, &(test_handle.output0),
				NULL, test_handle.mode, test_handle.ipu_handle);
	if (ret < 0) {
		printf("mxc_ipu_lib_task_init failed!\n");
		goto done;
	}

	while((done_cnt < test_handle.fcount) && (ctrl_c_rev == 0)) {
		if (fread(test_handle.ipu_handle->inbuf_start[next_update_idx], 1, test_handle.ipu_handle->ifr_size, file_in)
				< test_handle.ipu_handle->ifr_size) {
			ret = -1;
			printf("Can not read enough data from input file!\n");
			break;
		}
		if (first_time && (test_handle.mode & OP_STREAM_MODE)) {
			if (fread(test_handle.ipu_handle->inbuf_start[1], 1, test_handle.ipu_handle->ifr_size, file_in)
					< test_handle.ipu_handle->ifr_size) {
				ret = -1;
				printf("Can not read enough data from input file!\n");
				break;
			}
			first_time = 0;
			done_cnt++;
		}
		next_update_idx = mxc_ipu_lib_task_buf_update(test_handle.ipu_handle, 0, 0, 0, output_to_file_cb, &test_handle);
		if (next_update_idx < 0)
			break;
		done_cnt++;
	}

	mxc_ipu_lib_task_uninit(test_handle.ipu_handle);

done:
	fclose(file_in);
	if (test_handle.file_out0)
		fclose(test_handle.file_out0);
	if (test_handle.file_out1)
		fclose(test_handle.file_out1);

	system("echo 0,0 > /sys/class/graphics/fb0/pan");

	return ret;
}
