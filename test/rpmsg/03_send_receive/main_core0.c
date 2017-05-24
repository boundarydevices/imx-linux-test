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

#include <stdio.h>
#include "../common/common.h"

#define TC_TRANSFER_COUNT 12
#define DATA_LEN 45

int tc_send_receive(int fd)
{
	int result;

	result = tc_send(fd, 1, 1, TC_TRANSFER_COUNT, DATA_LEN);
	if (result != 0)
		return result;

	result = tc_receive(fd, 1, 1, TC_TRANSFER_COUNT, DATA_LEN);
	return result;
}

int main(void)
{
	int fd, result;

	fd = init();
	if (fd >= 0) {
		printf("/dev/ttyRPMSG successfully initialized\r\n");
		result = tc_send_receive(fd);
		deinit(fd);
		return result;
	}
	return fd;
}
