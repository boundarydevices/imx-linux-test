/*
 * Copyright 2008-2015 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <linux/mxc_sim_interface.h>
#include "../../include/soc_check.h"

#define BUFFER_LEN	200
#define T0_CMD_LEN	5
#define ILLEGLE_CMD	20

/* These command is application level. These
 * command are dumped from test program before.
 * CLA INS P1 P2 Lc DATA Le
 */
static unsigned char cmd0[7] = {0xa0, 0xa4, 0x00, 0x00, 0x02, 0x3f, 0x00};
static unsigned char cmd1[7] = {0xa0, 0xa4, 0x00, 0x00, 0x02, 0x2f, 0xe2};
static unsigned char cmd2[5] = {0xa0, 0xb0, 0x00, 0x00, 0x0a};
static unsigned char* cmd_array[3] = {cmd0, cmd1, cmd2};
static unsigned char cmd_array_size[3] = {7, 7, 5};

static int send_t0_cmd(int fd, char *cmd_buff, unsigned char cmd_len, char *rcv_buff)
{
	unsigned char remaining = cmd_len;
	unsigned char tx_len = 0;
	unsigned char tx_pos = 0;
	unsigned char rx_pos = 0;
	unsigned char i = 0;

	int ret;

	sim_xmt_t xmt_data;
	sim_rcv_t rcv_data;

	while (remaining != 0) {
		tx_len = (remaining > T0_CMD_LEN) ? T0_CMD_LEN : remaining;

		xmt_data.xmt_buffer = cmd_buff + tx_pos;
		xmt_data.xmt_length = tx_len;

		ret = ioctl(fd, SIM_IOCTL_XMT, &xmt_data);
		if (ret < 0)
			return ret;

		remaining -= tx_len;
		tx_pos += tx_len;

		rcv_data.rcv_length = 0;
		rcv_data.rcv_buffer = rcv_buff + rx_pos;

		ret = ioctl(fd, SIM_IOCTL_RCV, &rcv_data);
		if (ret < 0)
			return ret;

		rx_pos += rcv_data.rcv_length;

		/* illgal status byte */
		if (rcv_buff[0] == 0x6e && rcv_buff[1] == 0X00) {
			printf("CLA not support\n");
			return -ILLEGLE_CMD;
		} else if (rcv_buff[0] == 0x6d && rcv_buff[1] == 0X00) {
			printf("CLA support, but INS invalid\n");
			return -ILLEGLE_CMD;
		} else if (rcv_buff[0] == 0x6b && rcv_buff[1] == 0X00) {
			printf("CLA INS support, but P1P2 incorect\n");
			return -ILLEGLE_CMD;
		} else if (rcv_buff[0] == 0x67 && rcv_buff[1] == 0X00) {
			printf("CLA INS P1 P2 supported, but Lc incorrect\n");
			return -ILLEGLE_CMD;
		} else if (rcv_buff[0] == 0x6f && rcv_buff[1] == 0X00) {
			printf("cmd not supported and no precise diagnosis given\n");
			return -ILLEGLE_CMD;
		}
	}

	for (i = 0; i < rx_pos; i++)
		printf("rx[%d] = 0x%x ", i, rcv_buff[i]);
	printf("\n");

	return rx_pos;
}

int main(int argc, char *argv[])
{
	int sim_fd, ret;
	unsigned int i;
	sim_atr_t atr_data;
	sim_timing_t timing_data;
	unsigned int protocol = SIM_PROTOCOL_T0;
	sim_baud_t baud_data;
	unsigned char rx_buffer[BUFFER_LEN] = {0};
	char *soc_list[] = {"i.MX6UL", "i.MX7D", " "};

	ret = soc_version_check(soc_list);
	if (ret == 0) {
		printf("sim not supported on current soc\n");
		return 0;
	}

	sim_fd = open("/dev/mxc_sim", O_RDWR);
	if (sim_fd < 0) {
		printf("Error when opening file\n");
		return -1;
	}

	ret = ioctl(sim_fd, SIM_IOCTL_COLD_RESET);
	atr_data.atr_buffer = rx_buffer;

	ret = ioctl(sim_fd, SIM_IOCTL_GET_ATR, &atr_data);
	if (ret < 0)
		return ret;

	for (i = 0; i < atr_data.size; i++)
		printf("atr[%d]= 0x%x ", i, atr_data.atr_buffer[i]);
	printf("\n");

	/*Hardcode the setting*/
	timing_data.bgt = 0;
	if (argc == 1) {
		timing_data.wwt = 9600;
		timing_data.cgt = 0;
		timing_data.cwt = 0;
	}

	baud_data.di = 1;
	baud_data.fi = 1;

	ret = ioctl(sim_fd, SIM_IOCTL_SET_PROTOCOL, &protocol);
	ret = ioctl(sim_fd, SIM_IOCTL_SET_TIMING, &timing_data);
	ret = ioctl(sim_fd, SIM_IOCTL_SET_BAUD, &baud_data);

	for (i = 0; i < sizeof(cmd_array_size); i++) {
		ret = send_t0_cmd(sim_fd, cmd_array[i], cmd_array_size[i], rx_buffer);

		if (ret == -ILLEGLE_CMD)
			break;
		if (ret < 0) {
			printf("Error when tx/rx:%d\n", ret);
			break;
		}
	}
	close(sim_fd);

	return ret;
}

