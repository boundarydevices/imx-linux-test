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
 * @file mxc_ipudev_test.h
 *
 * @brief IPU device lib test implementation
 *
 * @ingroup IPU
 */
#ifndef __MXC_IPUDEV_TEST_H__
#define __MXC_IPUDEV_TEST_H__

#include <linux/ipu.h>
#include <linux/videodev.h>
#include "mxc_ipu_hl_lib.h"

typedef struct {
	ipu_lib_handle_t * ipu_handle;
	int fcount;
	int mode;
	int test_pattern;
	int block_width;
	int output1_enabled;
	char outfile0[128];
	char outfile1[128];
	FILE * file_out0;
	FILE * file_out1;
	ipu_lib_input_param_t input;
	ipu_lib_output_param_t output0;
	ipu_lib_output_param_t output1;
} ipu_test_handle_t;

extern int parse_config_file(char *file_name, ipu_test_handle_t *test_handle);
int run_test_pattern(int pattern, ipu_test_handle_t * test_handle);

#endif
