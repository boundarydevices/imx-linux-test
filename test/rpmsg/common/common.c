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

#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

struct termios ti;

int init(void)
{
	int fd;

	printf("Starting tests!\r\n");
	fd = open("/dev/ttyRPMSG", O_RDWR | O_NOCTTY);
	if (fd < 0) {
		printf("error %d\r\n", fd);
		return -1;
	}
	printf("open: %d\r\n", fd);

	tcflush(fd, TCIOFLUSH);

	if (tcgetattr(fd, &ti) < 0) {
		printf("error %d\r\n", fd);
		return -1;
	}

	cfmakeraw(&ti);
	cfsetospeed(&ti, B115200);
	cfsetispeed(&ti, B115200);
	if (tcsetattr(fd, TCSANOW, &ti) < 0) {
		printf("error %d\r\n", fd);
		return -1;
	}
	return fd;
}

void deinit(int fd)
{
	close(fd);
}

int pattern_cmp(char *buffer, char pattern, int len)
{
	int i;

	for (i = 0; i < len; i++)
		if (buffer[i] != pattern)
			return -1;
	return 0;
}

int tc_send(int fd, int test_case, int step, int transfer_count, int data_len)
{
	int result = 0, i;
	char data[data_len];

	memset(data, -1, data_len);

	for (i = 0; i < transfer_count; i++) {
		memset(data, i, data_len);
		result = write(fd, data, data_len);
		if (result != data_len) {
			printf("Sending message was unsuccessful.
				Send request (tc-step-loop) %d-%d-%d.\r\n",
				test_case, step, i);
			return -1;
		}
	}
	return 0;
}

int tc_receive(int fd, int test_case, int step, int transfer_count,
	int data_len)
{
	int result = 0, i;
	char data[data_len];

	memset(data, -1, data_len);

	for (i = 0; i < transfer_count; i++) {
		result = read(fd, data, data_len);
		if (result != data_len) {
			printf("Receiving message was unsuccessful.
				Receive request (tc-step-loop) %d-%d-%d.\r\n",
				test_case, step, i);
			return -1;
		}
		result = pattern_cmp(data, i, data_len);
		if (result == -1) {
			printf("Received bad message.
				Receive request (tc-step-loop) %d-%d-%d.\r\n",
				test_case, step, i);
			return result;
		}
	}
	return result;
}
