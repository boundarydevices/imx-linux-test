/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc. All rights reserved.
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
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

static int uart_speed(int s)
{
	switch (s) {
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	case 2000000:
		return B2000000;
	case 2500000:
		return B2500000;
	case 3000000:
		return B3000000;
	case 3500000:
		return B3500000;
	case 4000000:
		return B4000000;
	default:
		return B0;
	}
}

int set_speed(int fd, struct termios *ti, int speed)
{
	cfsetospeed(ti, uart_speed(speed));
	cfsetispeed(ti, uart_speed(speed));
	if (tcsetattr(fd, TCSANOW, ti) < 0) {
		perror("Can't set speed");
		return -1;
	}
	printf("Speed set to %d\n", speed);
	return 0;
}

int init_uart(char *dev, struct termios *ti)
{
	int fd;

	fd = open(dev, O_RDWR |  O_NOCTTY);
	if (fd < 0) {
		perror("Can't open serial port");
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	if (tcgetattr(fd, ti) < 0) {
		perror("Can't get port settings");
		return -1;
	}

	cfmakeraw(ti);
	printf("Serial port %s opened\n", dev);
	return fd;
}

int CHUNK_SIZE = 1024;
int CHUNKS = 100;

void real_op(int fd, char op)
{
	char tmp[1024];
	int i = 0, k = 0;
	int count, total = 0;
	char *opstr = (op == 'R' ? "reading" : "writing");

	/* init buffer */
	if (op == 'W') {
		for (i = 0; i < CHUNK_SIZE; i++)
			tmp[i] = (i + 1) % 0x100;
		i = 0;
	}

	while (total < (CHUNK_SIZE * CHUNKS) && k < 10) {
		i++;
		if (op == 'R') {
			/* clear it first */
			memset(tmp, 0, sizeof(tmp));
			count = read(fd, tmp, CHUNK_SIZE);
		} else
			count = write(fd, tmp, CHUNK_SIZE);

		if (count < 0) {
			printf("\nError while %s: %d attempt %d\n",
				opstr, (int)count, i);
			k++;
		} else {
			total += count;

			if (op == 'R') {
				int v;
				unsigned int dec_val = 0;

				for (v = 0; v < CHUNK_SIZE && v < count; v++)
					printf("byte%d : 0x%x\n", v, tmp[v]);

				if (count > 0) {
					printf("[%d] READ finished :%d bytes\n",
						i, count);
					for (v = 0; v < count; v++)
						dec_val |= (tmp[v] << (8 * v));
					printf("dec val is %08d.\n", dec_val);
				}
				else if (count == 0)
					printf("We read 0 byte\n");
			}
			if (count == 0)
				k++;
		}
	}
	close(fd);
	printf("\ndone : total : %d\n", total);

	if (k > 0)
		printf("too many errors or empty %ss\n", opstr);

	if (i == CHUNKS)
		printf("\t %s : %d calls issued, %d bytes per call\n",
			opstr, CHUNKS, CHUNK_SIZE);
	else
		printf("\t%d %s calls issued\n"
			"\t total : %d\n",
			i, opstr, total);
}

int main(int argc, char *argv[])
{
	if (argc < 5)
		printf("Usage:\n\t%s <PORT> <READ/WRITE> X Y\n"
			"X : the group number\n"
			"Y : the group size\n\n"
			"For example:\n"
			"mx6sx-sd(mcc and m4 are running), receive one byte:\n"
			"./mxc_mcc_tty_test.out /dev/ttyMCCp0 115200 R 1 1\n\n"
			,
			argv[0]);
	else {
		struct termios ti;
		int fd = init_uart(argv[1], &ti);

		CHUNKS = atoi(argv[4]);
		if (CHUNKS < 0) {
			printf("error chunks, %d\n", CHUNKS);
			return 1;
		}

		CHUNK_SIZE  = atoi(argv[5]);

		if (fd >= 0 && !set_speed(fd, &ti, atoi(argv[2]))) {
			char op = argv[3][0] & (~0x20);
			real_op(fd, op);
		}
	}
	return 0;
}
