/*
 * Copyright 2009-2011 Freescale Semiconductor, Inc. All Rights Reserved.
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

typedef struct {
	struct ipu_task task;
	int fcount;
	int loop_cnt;
	int show_to_fb;
	char outfile[128];
} ipu_test_handle_t;

extern int parse_config_file(char *file_name, ipu_test_handle_t *test_handle);

#endif
