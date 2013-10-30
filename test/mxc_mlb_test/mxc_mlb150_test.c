/*
 * Copyright (C) 2011-2013 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
/**
 * MLB driver unit test file
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <linux/mxc_mlb.h>

#define MLB_SYNC_DEV	"/dev/sync"
#define MLB_CTRL_DEV	"/dev/ctrl"
#define MLB_ASYNC_DEV	"/dev/async"
#define MLB_ISOC_DEV	"/dev/isoc"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define vprintf(fmt, args...) \
do {	\
	if (verbose)	\
		printf(fmt, ##args);	\
} while (0);

static int m_fd;
static int verbose = 0;
static unsigned int fps = 512;
static int blocked = 0;
static int t_case = 0;
static unsigned int sync_quad = 0;
static unsigned int isoc_pkg_len = 0;

int do_basic_test(int fd);
int do_txrx_test(int fd);

void print_help(void)
{
	printf("Usage: mxc_mlb150_test [-v] [-h] [-b] [-f fps] [-t casetype] [-q sync quadlets] [-p isoc packet length]\n"
	"\t-v verbose\n\t-h help\n\t-b block io test\n"
	"\t-f FPS, 256/512/1024/2048/3072/4096/6144\n"
	"\t-t CASE, CASE can be 'sync', 'ctrl', 'async', 'isoc'\n"
	"\t-q SYNC QUADLETS, quadlets per frame in sync mode, can be 1, 2, or 3\n"
	"\t-p Packet length, package length in isoc mode, can be 188 or 196\n");
}

int main(int argc, char *argv[])
{
	int ret, flags;
	char test_case_str[10] = { 0 };

	while (1) {
		ret = getopt(argc, argv, "vf:t:hbq:p:");

		if (1 == argc) {
			print_help();
			return 0;
		}

		if (-1 == ret)
			break;

		switch (ret) {
		case 'v':
			verbose = 1;
			break;
		case 'b':
			blocked = 1;
			break;
		case 't':
			if (strcmp(optarg, "sync") == 0) {
				t_case = 0;
			} else if (strcmp(optarg, "ctrl") == 0) {
				t_case = 1;
			} else if (strcmp(optarg, "async") == 0) {
				t_case = 2;
			}else if (strcmp(optarg, "isoc") == 0) {
				t_case = 3;
			} else {
				fprintf(stderr, "No such case\n");
				print_help();
				return 0;
			}
			strncpy(test_case_str, optarg, 5);
			break;
		case 'f':
			if (1 == sscanf(optarg, "%d", &fps)) {
				if (fps == 256 || fps == 512 || fps == 1024 ||
					fps == 2048 || fps == 3072 || fps == 4096 ||
					fps == 6144)
					break;
			}
			fprintf(stderr, "invalid input frame rate\n");
			return -1;
		case 'q':
			if (1 == sscanf(optarg, "%d", &sync_quad)) {
				if (sync_quad > 0 && sync_quad <= 3)
					break;
			}
			fprintf(stderr, "invalid sync quadlets\n");
			return -1;
		case 'p':
			if (1 == sscanf(optarg, "%d", &isoc_pkg_len)) {
				if (isoc_pkg_len == 188 || isoc_pkg_len == 196)
					break;
			}
			fprintf(stderr, "invalid isoc package length\n");
			return -1;
		case 'h':
			print_help();
			return 0;
		default:
			fprintf(stderr, "invalid command line\n");
			return -1;
		}
	}

	flags = O_RDWR;
	if (!blocked)
		flags |= O_NONBLOCK;

	switch (t_case) {
	case 0:
		m_fd = open(MLB_SYNC_DEV, flags);
		break;
	case 1:
		m_fd = open(MLB_CTRL_DEV, flags);
		break;
	case 2:
		m_fd = open(MLB_ASYNC_DEV, flags);
		break;
	case 3:
		m_fd = open(MLB_ISOC_DEV, flags);
		break;
	default:
		break;
	}

	if (m_fd <= 0) {
		fprintf(stderr, "Failed to open device\n");
		return -1;
	}

	if (do_basic_test(m_fd)) {
		printf("Basic test failed\n");
		return -1;
	}

	printf("=======================\n");
	ret = 0;
	printf("%s tx/rx test start\n", test_case_str);
	if (do_txrx_test(m_fd))
		ret = -1;
	printf("%s tx/rx test %s\n", test_case_str, (ret ? "failed" : "PASS"));

	close(m_fd);

	return ret;
}

int do_basic_test(int fd)
{
	int ret;
	unsigned long ver;
	unsigned char addr = 0xC0;

	/* ioctl check */
	/* get mlb device version */
	ret = ioctl(fd, MLB_GET_VER, &ver);
	if (ret) {
		printf("Get version failed: %d\n", ret);
		return -1;
	}
	printf("MLB device version: 0x%08x\n", (unsigned int)ver);

	/* set mlb device address */
	ret = ioctl(fd, MLB_SET_DEVADDR, &addr);
	if (ret) {
		printf("Set device address failed %d\n", ret);
		return -1;
	}
	printf("MLB device address: 0x%x\n", addr);

	return 0;
}

void dump_hex(const char *buf, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		vprintf("%02x ", *(buf + i));
		if ((i+1) % 8 == 0)
			vprintf("\n");
	}
	if (i % 8 != 0)
		vprintf("\n");
}

int do_txrx_test(int fd)
{
	int ret, packets = 5000;
	fd_set rdfds, wtfds;
	unsigned long evt;
	char buf[2048];
	int gotlen = 0;

	/**
	 * set channel address
	 * MSB is for tx, LSB is for rx
	 * the normal address range [0x02, 0x3E]
	 */

	ret = ioctl(fd, MLB_SET_FPS, &fps);
	if (ret) {
		printf("Set fps to 512 failed: %d\n", ret);
		return -1;
	}
	printf("Set fps to %d\n", fps);

	switch (t_case) {
	case 0:
		evt = 0x02 << 16 | 0x01;
		break;
	case 1:
		evt = 0x04 << 16 | 0x03;
		break;
	case 2:
		evt = 0x06 << 16 | 0x05;
		break;
	case 3:
		evt = 0x08 << 16 | 0x07;
		break;
	default:
		break;
	}

	ret = ioctl(fd, MLB_CHAN_SETADDR, &evt);
	if (ret) {
		printf("set control channel address failed: %d\n", ret);
		return -1;
	}

	if (0 != sync_quad) {
		ret = ioctl(fd, MLB_SET_SYNC_QUAD, &sync_quad);
		if (ret) {
			printf("Set quadlets of frame for sync mode failed: %d\n", ret);
			return -1;
		}
	}

	if (0 != isoc_pkg_len) {
		switch (isoc_pkg_len) {
		case 188:
			ret = ioctl(fd, MLB_SET_ISOC_BLKSIZE_188);
			break;
		case 196:
			ret = ioctl(fd, MLB_SET_ISOC_BLKSIZE_196);
			break;
		default:
			ret = -1;
			printf("Invalid block size for isoc mode\n");
			break;
		}
		if (ret) {
			printf("Set block size for isoc mode failed: %d\n", ret);
			return -1;
		}
	}

	/* channel startup */
	ret = ioctl(fd, MLB_CHAN_STARTUP);
	if (ret) {
		printf("Failed to startup channel\n");
		return -1;
	}

	if (blocked) {
		while (1) {
			ret = read(fd, buf, 2048);
			if (ret <= 0) {
				printf("Failed to read MLB packet: %s\n", strerror(errno));
			} else {
				vprintf(">> Read MLB packet:\n(length)\n%d\n(data)\n", ret);
				dump_hex(buf, ret);
			}

			ret = write(fd, buf, ret);
			if (ret <= 0) {
				printf("Write MLB packet failed %d: %s\n", ret, strerror(errno));
			} else {
				vprintf("<< Send this received MLB packet back\n");
				if (t_case > 0)
					packets --;
			}

			if (!packets)
				break;
		}

		goto shutdown;
	}

	while (1) {

		FD_ZERO(&rdfds);
		FD_ZERO(&wtfds);

		FD_SET(fd, &rdfds);
		FD_SET(fd, &wtfds);

		ret = select(fd + 1, &rdfds, &wtfds, NULL, NULL);

		if (ret < 0) {
			printf("Select failed: %d\n", ret);
			return -1;
		} else if (ret == 0) {
			continue;
		}

		if (FD_ISSET(fd, &rdfds)) {
			/* check event */
			ret = ioctl(fd, MLB_CHAN_GETEVENT, &evt);
			if (ret == 0) {
				vprintf("## Get event: %x\n", (unsigned int)evt);
			} else {
				if (errno != EAGAIN) {
					printf("Failed to get event:%s\n", strerror(errno));
					return -1;
				}
			}

			/* check read */
			ret = read(fd, buf, 2048);
			if (ret <= 0) {
				if (errno != EAGAIN)
					printf("Failed to read MLB packet: %s\n", strerror(errno));
			} else {
				vprintf(">> Read MLB packet:\n(length)\n%d\n(data)\n", ret);
				dump_hex(buf, ret);
				gotlen = ret;
			}
		}

		/* check write */
		if (FD_ISSET(fd, &wtfds) && gotlen) {
			ret = write(fd, buf, gotlen);
			if (ret <= 0) {
				printf("Write MLB packet failed\n");
			} else {
				vprintf("<< Send this received MLB packet back\n");
				if (t_case > 0)
					packets --;
			}
			gotlen = 0;
		}

		if (!packets)
			break;
	}

shutdown:
	usleep(1000);
	ret = ioctl(fd, MLB_CHAN_SHUTDOWN);
	if (ret) {
		printf("Failed to shutdown async channel\n");
		return -1;
	}

	return 0;
}
