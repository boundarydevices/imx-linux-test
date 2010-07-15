/*
 * Copyright 2009-2010 Freescale Semiconductor, Inc. All Rights Reserved.
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

	if (test_handle->file_out)
		if(fwrite(test_handle->ipu_handle->outbuf_start[index], 1,
				test_handle->ipu_handle->ofr_size,
				test_handle->file_out) < test_handle->ipu_handle->ofr_size) {
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
		} else if (strcmp(argv[i], "-Q") == 0)
			test_handle->query_task = 1;
		else if (strcmp(argv[i], "-K") == 0) {
			test_handle->kill_task = 1;
			test_handle->kill_task_idx = atoi(argv[++i]);
		}
	}

	if (test_handle->test_pattern)
		return 0;
	if (test_handle->query_task || test_handle->kill_task)
		return 0;

	if ((test_handle->input.width == 0) || (test_handle->input.height == 0) ||
			(test_handle->output.width == 0) ||
			(test_handle->output.height == 0)
			|| (test_handle->fcount < 1))
		return -1;

	return 0;
}

int query_ipu_task(void)
{
	int i;
	ipu_lib_ctl_task_t task;

	for (i = 0; i< MAX_TASK_NUM; i++) {
		task.index = i;
		mxc_ipu_lib_task_control(IPU_CTL_TASK_QUERY, (void *)(&task), NULL);
		if (task.task_pid) {
			printf("\ntask %d:\n", i);
			printf("\tpid: %d\n", task.task_pid);
			printf("\tmode:\n");
			if (task.task_mode & IC_ENC)
				printf("\t\tIC_ENC\n");
			if (task.task_mode & IC_VF)
				printf("\t\tIC_VF\n");
			if (task.task_mode & IC_PP)
				printf("\t\tIC_PP\n");
			if (task.task_mode & ROT_ENC)
				printf("\t\tROT_ENC\n");
			if (task.task_mode & ROT_VF)
				printf("\t\tROT_VF\n");
			if (task.task_mode & ROT_PP)
				printf("\t\tROT_PP\n");
			if (task.task_mode & VDI_IC_VF)
				printf("\t\tVDI_IC_VF\n");
		}
	}

	return 0;
}

int kill_ipu_task(int index)
{
	ipu_lib_ctl_task_t task;

	task.index = index;
	mxc_ipu_lib_task_control(IPU_CTL_TASK_KILL, (void *)(&task), NULL);

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0, next_update_idx = 0, done_cnt = 0, first_time = 1;
	int done_loop = 0, total_cnt = 0;
	ipu_lib_handle_t ipu_handle;
	ipu_test_handle_t test_handle;
	FILE * file_in = NULL;
	struct sigaction act;
	struct timeval begin, end;
	int sec, usec, run_time = 0;

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
				"<input raw file>\n\n" \
				"Query ipu task runing:\n" \
				"-Q\n\n" \
				"Kill ipu task:\n" \
				"-K <task_index>\n", argv[0]);
		printf("\ntest pattern:\n" \
			"1: video pattern with user define dma buffer queue, one full-screen output\n" \
			"2: hopping block screen save\n" \
			"3: color bar + hopping block\n" \
			"4: color bar IC global alpha overlay\n" \
			"5: color bar IC separate local alpha overlay\n" \
			"6: color bar IC local alpha within pixel overlay\n" \
			"7: ipu dma copy test\n" \
			"8: 2 screen layer test using IC global alpha blending\n" \
			"9: 3 screen layer test using IC global alpha blending\n" \
			"10: 2 screen layer test using IC local alpha blending with alpha value in separate buffer\n" \
			"11: 3 screen layer test using IC local alpha blending with alpha value in separate buffer\n" \
			"12: 2 screen layer test using IC local alpha blending with alpha value in pixel\n" \
			"13: 3 screen layer test using IC local alpha blending with alpha value in pixel\n" \
			"14: 2 screen layer test IPC ProcessA + ProcessB with globla alpha blending\n" \
			"15: 2 screen layer test IPC ProcessA + ProcessB with local alpha blending\n" \
			"16: 3 screen layer test IPC ProcessA(first_layer + sencond_layer) + ProcessB(third_layer) with globla alpha blending\n" \
			"17: 3 screen layer test IPC ProcessA(first_layer + sencond_layer) + ProcessB(third_layer) with local alpha blending\n" \
			"18: 3 screen layer test IPC ProcessA(first_layer) ProcessB(sencond_layer) ProcessC(third_layer) with local alpha blending\n" \
			"19: 2 screen layer test IPC ProcessA(first_layer) ProcessB(sencond_layer) with local alpha blending plus tv copy\n\n");
		return -1;
	}

	system("echo 0,0 > /sys/class/graphics/fb0/pan");

	if (test_handle.test_pattern) {
		ret = run_test_pattern(test_handle.test_pattern, &test_handle);
		system("echo 0,0 > /sys/class/graphics/fb0/pan");
		return ret;
	} else if (argc < 4) {
		if (test_handle.query_task)
			return query_ipu_task();
		else if (test_handle.kill_task)
			return kill_ipu_task(test_handle.kill_task_idx);
		else {
			printf("Pls set input file\n");
			return -1;
		}
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

	if (test_handle.outfile && !test_handle.output.show_to_fb)
		test_handle.file_out = fopen(test_handle.outfile, "wb");

	ret = mxc_ipu_lib_task_init(&(test_handle.input), NULL, &(test_handle.output),
			test_handle.mode, test_handle.ipu_handle);
	if (ret < 0) {
		printf("mxc_ipu_lib_task_init failed!\n");
		goto done;
	}

again:
	while((done_cnt < test_handle.fcount) && (ctrl_c_rev == 0)) {
		gettimeofday(&begin, NULL);
		if (fread(test_handle.ipu_handle->inbuf_start[next_update_idx], 1, test_handle.ipu_handle->ifr_size, file_in)
				< test_handle.ipu_handle->ifr_size) {
			ret = -1;
			printf("Can not read enough data from input file!\n");
			break;
		}
		if (first_time && (test_handle.mode == (TASK_VDI_VF_MODE | OP_NORMAL_MODE)) && (test_handle.input.motion_sel != HIGH_MOTION)) {
			if (fread(test_handle.ipu_handle->inbuf_start[1], 1, test_handle.ipu_handle->ifr_size, file_in)
					< test_handle.ipu_handle->ifr_size) {
				ret = -1;
				printf("Can not read enough data from input file!\n");
				break;
			}
			first_time = 0;
			done_cnt++;
			total_cnt++;
		}
		if (first_time && (test_handle.mode & OP_STREAM_MODE)) {
			if (fread(test_handle.ipu_handle->inbuf_start[1], 1, test_handle.ipu_handle->ifr_size, file_in)
					< test_handle.ipu_handle->ifr_size) {
				ret = -1;
				printf("Can not read enough data from input file!\n");
				break;
			}
			if ((test_handle.mode & TASK_VDI_VF_MODE) && (test_handle.input.motion_sel != HIGH_MOTION)) {
				if (fread(test_handle.ipu_handle->inbuf_start[2], 1, test_handle.ipu_handle->ifr_size, file_in)
						< test_handle.ipu_handle->ifr_size) {
					ret = -1;
					printf("Can not read enough data from input file!\n");
					break;
				}
				done_cnt++;
				total_cnt++;
			}
			first_time = 0;
			done_cnt++;
			total_cnt++;
		}
		next_update_idx = mxc_ipu_lib_task_buf_update(test_handle.ipu_handle, 0, 0, 0, output_to_file_cb, &test_handle);
		if (next_update_idx < 0)
			break;
		done_cnt++;
		total_cnt++;

		gettimeofday(&end, NULL);
		sec = end.tv_sec - begin.tv_sec;
		usec = end.tv_usec - begin.tv_usec;

		if (usec < 0) {
			sec--;
			usec = usec + 1000000;
		}
		run_time += (sec * 1000000) + usec;
	}

	done_loop++;
	if ((done_loop < test_handle.loop_cnt) && (ctrl_c_rev == 0)) {
		done_cnt = 0;
		fseek(file_in, 0L, SEEK_SET);
		goto again;
	}

	printf("total frame count %d avg frame time %d us, fps %f\n", total_cnt, run_time/total_cnt, total_cnt/(run_time/1000000.0));

	mxc_ipu_lib_task_uninit(test_handle.ipu_handle);

done:
	fclose(file_in);
	if (test_handle.file_out)
		fclose(test_handle.file_out);

	system("echo 0,0 > /sys/class/graphics/fb0/pan");

	return ret;
}
