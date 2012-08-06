/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc. All rights reserved.
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

int set_flow_control(int fd, struct termios *ti, int flow_control)
{
	if (flow_control)
		ti->c_cflag |= CRTSCTS;
	else
		ti->c_cflag &= ~CRTSCTS;

	if (tcsetattr(fd, TCSANOW, ti) < 0) {
		perror("Can't set flow control");
		return -1;
	}
	printf("%sUsing flow control\n", (flow_control? "" : "NOT "));
	return 0;
}

int init_uart(char *dev, struct termios *ti)
{
	int fd, i;
	unsigned long flags = 0;

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
#define STR(X) #X
#define XSTR(X) STR(X)


int CHUNK_SIZE = 1024;
int CHUNKS = 100;

#define RECORDS	20
static int record[RECORDS];

void real_op(int fd, char op)
{
	int nfds;
	char tmp[1024];
	int i = 0, j = -1, k = 0, r;
	size_t count, total = 0, max = 0, min = 0;
	char *opstr = (op == 'R' ? "reading" : "writing");
	fd_set rfds;
	struct timeval tv;

	nfds = fd;
	tv.tv_sec = 15;
	tv.tv_usec = 0;

	printf("Hit enter to start %s\n", opstr);
	read(STDIN_FILENO, tmp, 3);

	/* init buffer */
	if (op == 'W') {
		for (i = 0; i < CHUNK_SIZE; i++)
			tmp[i] = (i + 1) % 0x100;
		i = 0;
	}

	while (total < (CHUNK_SIZE * CHUNKS) && k < 10)
	{
		i++;
		if (op == 'R') {
			/*
			FD_ZERO(&rfds);
			FD_SET(fd, &rfds);
			r = select(nfds + 1, &rfds, NULL, NULL, &tv);
			*/
			r = 1;
			/* clear it first */
			memset(tmp, 0, sizeof(tmp));
			if (r > 0)
				count = read(fd, tmp, CHUNK_SIZE);
			else {
				printf("timeout\n");
				count = 0;
			}
		} else
			count = write(fd, tmp, CHUNK_SIZE);

#ifdef ANDROID
		if (count > SSIZE_MAX)
#else
		if (count < 0)
#endif
		{
			printf("\nError while %s: %d attempt %d\n",
				opstr, (int)count, i);
			k++;
		} else {
			total += count;

			if (op == 'R') {
				int v;
				static int sum = 0;
				int bad = 0;

				for (v = 0; v < CHUNK_SIZE && v < count; v++, sum++) {
					if (tmp[v] == (((sum % CHUNK_SIZE) + 1) % 0x100)) {
						continue;
					} else {
						bad = 1;
						printf("%d (should be %x): but %x\n",
							v, ((sum % CHUNK_SIZE) + 1) % 0x100, tmp[v]);
					}
				}

				if (count > 0)
					printf(":)[ %d ] READ is %s, count : %d\n",
						i, bad ? "bad!!!" : "good.", count);
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
		printf("\t%d %s calls isued\n"
			"\t total : %d\n",
			i, opstr, total);
}

int main(int argc, char* argv[])
{
	if (argc < 6)
		printf("Usage:\n\t%s <PORT> <BAUDRATE> <FLOWCONTROL> <READ/WRITE> X Y\n"
			"X : the group number\n"
			"Y : the group size\n\n"
			"For example:\n"
			"in mx23, send one byte:\n"
			"         ./uart2 /dev/ttySP1 115200 F W 1 1\n\n"
			"in mx28, receive one byte:\n"
			"         ./uart2 /dev/ttySP0 115200 F R 1 1\n\n"
			"in mx53-evk(with the extension card), receive one byte:\n"
			"         ./uart2 /dev/ttymxc1 115200 F R 1 1\n\n"
			,
			argv[0]);
	else {
		struct termios ti;
		int fd = init_uart(argv[1], &ti);

		CHUNKS = atoi(argv[5]);
		if (CHUNKS < 0) {
			printf("error chunks, %d\n", CHUNKS);
			return 1;
		}

		CHUNK_SIZE  = atoi(argv[6]);

		if (fd >= 0 && !set_speed(fd, &ti, atoi(argv[2]))
			&& !set_flow_control(fd, &ti, ((argv[3][0] & (~0x20))=='F')))
		{
			char op = argv[4][0] & (~0x20);
			real_op(fd, op);
		}
	}
	return 0;
}
