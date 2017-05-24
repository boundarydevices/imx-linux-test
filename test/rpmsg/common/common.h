/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __COMMON_H__
#define __COMMON_H__

int init(void);

void deinit(int fd);

int pattern_cmp(char *buffer, char pattern, int len);

int tc_send(int fd, int test_case, int step, int transfer_count, int data_len);

int tc_receive(int fd, int test_case, int step, int transfer_count,
	int data_len);

#endif
