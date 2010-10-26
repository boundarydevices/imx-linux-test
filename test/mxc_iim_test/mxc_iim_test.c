/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 * Terry Lv <r65388@freescale.com>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static void print_usage()
{
	printf("Usage:\n"
		"Read iim: mxc_iim_test read -d <addr>\n"
		"Write iim: mxc_iim_test fuse -d <addr> -v <value>\n"
		"Note: All value are in hex format!\n");
}

int main(int argc, char *argv[])
{
	int fd, ch;
	int is_read = -1;
	unsigned int bankaddr;
	unsigned int value;
	char ch_val;
	char *end_char = NULL;

	if (argc < 4 || argc > 6)
		goto invalid_param_out;

	if (strcmp(argv[1], "read") == 0) {
		if (argc != 4)
			goto invalid_param_out;
		is_read = 1;
	} else if (strcmp(argv[1], "blow") == 0) {
		if (argc != 6)
			goto invalid_param_out;
		is_read = 0;
	} else {
		printf("Unsupported option: %s!\n", argv[1]);
		goto invalid_param_out;
	}

	while (-1 != (ch = getopt(argc - 1, &argv[1], "d:v:"))) {
		switch (ch) {
		case 'd':
			bankaddr = strtoul(optarg, &end_char, 16);
			if (*end_char)
				goto invalid_param_out;
			break;
		case 'v':
			value = strtoul(optarg, &end_char, 16);
			if (*end_char)
				goto invalid_param_out;
			break;
		default:
			printf("Unsupported option: -%c!\n", ch);
			goto invalid_param_out;
			break;
		}
	}

	if (-1 == is_read)
		goto invalid_param_out;

	fd = open("/dev/mxc_iim", O_RDWR);
	if (-1 == fd) {
		printf("Can't open /dev/mxc_iim!\n");
		return -1;
	}

	if (bankaddr) {
		if (-1 == lseek(fd, bankaddr, SEEK_SET)) {
			printf("lseek failed at bank addr 0x%08x\n",
				bankaddr);
			goto op_err_out;
		}
	}

	ch_val = (char)value;

	if (is_read) {
		if (-1 == read(fd, &ch_val, sizeof(char))) {
			printf("Read failed at bank addr 0x%08x\n",
				bankaddr);
			goto op_err_out;
		}
		printf("Value at bank addr 0x%08x: 0x%x\n",
			bankaddr, ch_val);
	} else {
		if (-1 == write(fd, &ch_val, sizeof(char))) {
			printf("Write failed at bank addr 0x%08x, "
				"value: 0x%08x\n",
				bankaddr, ch_val);
			goto op_err_out;
		}
		printf("Writting 0x%08x to bank addr 0x%08x\n",
			ch_val, bankaddr);
		if (-1 == lseek(fd, bankaddr, SEEK_SET)) {
			printf("lseek failed at bank addr 0x%08x\n",
				bankaddr);
			goto op_err_out;
		}
		if (-1 == read(fd, &ch_val, sizeof(char))) {
			printf("Read failed at bank addr 0x%08x\n",
				bankaddr);
			goto op_err_out;
		}
		printf("Value at bank addr 0x%08x: 0x%08x\n",
			bankaddr, ch_val);
	}

	close(fd);

	return 0;
op_err_out:
	return -2;
invalid_param_out:
	printf("Invalid parameters!\n");
	print_usage();
	return -1;
}

