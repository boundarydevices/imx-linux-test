/*
 * Copyright 2006 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <stropts.h>
#include <poll.h>

#define DEVICE_NAME 			"/dev/ipctest"
/* MU channels */
#define IPC_CHANNEL0 			"/dev/mxc_ipc/0"
#define IPC_CHANNEL1 			"/dev/mxc_ipc/1"
#define IPC_CHANNEL2 			"/dev/mxc_ipc/2"
/* SDMA DATA Channel */
#define IPC_CHANNEL3 			"/dev/mxc_ipc/3"
/* SDMA Log Channel */
#define IPC_CHANNEL4 			"/dev/mxc_ipc/4"
#define IPC_CHANNEL5 			"/dev/mxc_ipc/5"

#define MAX_BUFFER_SIZE			8192

#define TRANFER_SIZE			8192


struct ioctl_args {
	int iterations;
	unsigned short channel;
	unsigned short vchannel;
	unsigned short bytes;
	unsigned short messages;
};

char wbuf[MAX_BUFFER_SIZE];
char rbuf[MAX_BUFFER_SIZE];

int check_data_integrity(char *buf1, char *buf2, int count)
{
	int result = 0;
	int i;

	for (i = 0; i < count; i++) {
		if (buf1[i] != buf2[i]) {
			printf("Corrupted data at %d wbuf = %d rbuf = %d\n", i,
			       buf1[i], buf2[i]);
			result = -1;
		}
	}

	return result;
}

void fill_buffer(char *buffer, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		buffer[i] = (char)i;
	}
}

void test1(void)
{
	int fd = -1;

	printf("TEST 1: Try to open %s channel (short message)\n",
	       IPC_CHANNEL0);
	printf("press any key to start test\n");
	(void)getchar();

	fd = open(IPC_CHANNEL0, O_RDWR);
	if (fd == -1) {
		printf("test 1 failed with error %d\n", errno);
		return;
	}

	close(fd);

	fd = -1;
	printf("Try to open %s channel (short message)\n", IPC_CHANNEL1);
	fd = open(IPC_CHANNEL1, O_RDWR);
	if (fd == -1) {
		printf("test 1 failed with error %d\n", errno);
		return;
	}

	close(fd);

	fd = -1;
	printf("Try to open %s channel (short message)\n", IPC_CHANNEL2);
	fd = open(IPC_CHANNEL2, O_RDWR);
	if (fd == -1) {
		printf("test 1 failed with error %d\n", errno);
		return;
	}

	close(fd);

	fd = -1;
	printf("Try to open %s channel (packet data)\n", IPC_CHANNEL3);
	fd = open(IPC_CHANNEL3, O_RDWR);
	if (fd == -1) {
		printf("test 1 failed with error %d\n", errno);
		return;
	}

	close(fd);

	fd = -1;
	printf("Try to open %s channel (logging)\n", IPC_CHANNEL4);
	fd = open(IPC_CHANNEL4, O_RDONLY);
	if (fd == -1) {
		printf("test 1 failed with error %d\n", errno);
		return;
	}

	close(fd);

	fd = -1;
	printf("Try to open %s channel (logging)\n", IPC_CHANNEL5);
	fd = open(IPC_CHANNEL5, O_RDONLY);
	if (fd == -1) {
		printf("test 1 failed with error %d\n", errno);
		return;
	}

	close(fd);

	printf("test 1 passed\n");
}

void test2(void)
{
	int fd[6] = { -1, -1, -1, -1, -1, -1 };

	printf("TEST 2. Open all IPC channels at once and closed them\n");
	printf("press any key to start test\n");
	(void)getchar();

	fd[0] = open(IPC_CHANNEL0, O_RDWR);
	if (fd[0] == -1) {
		printf("test 2 failed with error %d for %s\n", errno,
		       IPC_CHANNEL0);
		return;
	}

	fd[1] = open(IPC_CHANNEL1, O_RDWR);
	if (fd[1] == -1) {
		printf("test 2 failed with error %d for %s\n", errno,
		       IPC_CHANNEL1);
		printf("test 2 failed with error %d\n", errno);
		return;
	}

	fd[2] = open(IPC_CHANNEL2, O_RDWR);
	if (fd[2] == -1) {
		printf("test 2 failed with error %d for %s\n", errno,
		       IPC_CHANNEL2);
		printf("test 2 failed with error %d\n", errno);
		return;
	}

	fd[3] = open(IPC_CHANNEL3, O_RDWR);
	if (fd[3] == -1) {
		printf("test 2 failed with error %d for %s\n", errno,
		       IPC_CHANNEL3);
		printf("test 2 failed with error %d\n", errno);
		return;
	}

	fd[4] = open(IPC_CHANNEL4, O_RDONLY);
	if (fd[4] == -1) {
		printf("test 2 failed with error %d for %s\n", errno,
		       IPC_CHANNEL4);
		printf("test 2 failed with error %d\n", errno);
		return;
	}

	fd[5] = open(IPC_CHANNEL5, O_RDONLY);
	if (fd[5] == -1) {
		printf("test 2 failed with error %d for %s\n", errno,
		       IPC_CHANNEL5);
		printf("test 2 failed with error %d\n", errno);
		return;
	}

	close(fd[0]);
	close(fd[1]);
	close(fd[2]);
	close(fd[3]);
	close(fd[4]);
	close(fd[5]);

	printf("TEST 2 OK\n");
}

void test3(void)
{
	int count, result, iterations;
	int fd = -1;
	int i;

	printf("TEST 3. Open IPC channel 0 and send one 32-bits message\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(IPC_CHANNEL0, O_RDWR);
	if (fd == -1) {
		printf("TEST 3 failed with error %d\n", errno);
		return;
	}
	//printf("Channel opened\n");

	iterations = 1;

	for (i = 0; i < iterations; i++) {
		fill_buffer(wbuf, 4);
		count = write(fd, wbuf, 4);
		if (count < 0) {
			printf
			    ("TEST 3 Write FAILED on channel %s iteration %d\n",
			     IPC_CHANNEL0, i);
			goto out;
		}

		count = read(fd, rbuf, count);
		if (count < 0) {
			printf
			    ("TEST 3 Read FAILED on channel %s iteration %d\n",
			     IPC_CHANNEL0, i);
			goto out;
		}

		result = check_data_integrity(wbuf, rbuf, count);
		if (result == -1) {
			printf("TEST 3 FAILED on channel %s iteration %d\n",
			       IPC_CHANNEL0, i);
			goto out;
		}

	}

	printf("TEST 3 OK\n");
      out:
	close(fd);
}

void test4(void)
{
	int count, result, iterations;
	int fd = -1;
	int i;

	printf("TEST 4. Open IPC channel 1 and send one 32-bits message\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(IPC_CHANNEL1, O_RDWR);
	if (fd == -1) {
		printf("TEST 4 failed with error %d\n", errno);
		return;
	}
	//printf("Channel opened\n");

	iterations = 10;

	for (i = 0; i < iterations; i++) {
		fill_buffer(wbuf, 4);
		count = write(fd, wbuf, 4);
		if (count < 0) {
			printf
			    ("TEST 4 Write FAILED on channel %s iteration %d\n",
			     IPC_CHANNEL1, i);
			goto out4;
		}
		//printf("wrote 4 bytes on channel\n");

		//sleep(1);

		count = read(fd, rbuf, count);
		if (count < 0) {
			printf
			    ("TEST 4 Read FAILED on channel %s iteration %d\n",
			     IPC_CHANNEL1, i);
			goto out4;
		}
		//printf("read 4 bytes from channel\n");

		result = check_data_integrity(wbuf, rbuf, count);
		if (result == -1) {
			printf("TEST 4 FAILED on channel %s iteration %d\n",
			       IPC_CHANNEL1, i);
			goto out4;
		}

	}

	printf("TEST 4 OK\n");
      out4:
	close(fd);

}

void test5(void)
{
	int count, result, iterations;
	int fd = -1;
	int i;

	printf("TEST 5. Open IPC channel 2 and send one 32-bits message\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(IPC_CHANNEL2, O_RDWR);
	if (fd == -1) {
		printf("TEST 5 failed with error %d\n", errno);
		return;
	}
	//printf("Channel opened\n");

	iterations = 10;

	for (i = 0; i < iterations; i++) {
		fill_buffer(wbuf, 4);
		count = write(fd, wbuf, 4);
		if (count < 0) {
			printf
			    ("TEST 5 Write FAILED on channel %s iteration %d\n",
			     IPC_CHANNEL2, i);
			break;
		}
		//printf("wrote 4 bytes on channel\n");

		//sleep(1);

		count = read(fd, rbuf, count);
		if (count < 0) {
			printf
			    ("TEST 5 Read FAILED on channel %s iteration %d\n",
			     IPC_CHANNEL2, i);
			break;
		}
		//printf("read 4 bytes from channel\n");

		result = check_data_integrity(wbuf, rbuf, count);
		if (result == -1) {
			printf("TEST 5 FAILED on channel %s iteration %d\n",
			       IPC_CHANNEL2, i);
			break;
		}

	}

	close(fd);

	printf("TEST 5 OK\n");
}

void test6(void)
{
	int count, count1, result, iterations;
	int fd = -1;
	int i;
	char *tmp_buf;

	printf("TEST 6. Open IPC channel 3 and send data\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(IPC_CHANNEL3, O_RDWR);
	if (fd == -1) {
		printf("TEST 6 failed with error %d\n", errno);
		return;
	}

	iterations = 10;

	for (i = 0; i < iterations; i++) {
		fill_buffer(wbuf, TRANFER_SIZE);

		count = write(fd, wbuf, TRANFER_SIZE);
		if (count < 0) {
			printf
			    ("TEST 6 Write FAILED on channel %s iteration %d\n",
			     IPC_CHANNEL3, i);
			break;
		}
		tmp_buf = rbuf;
		while (count > 0) {
			count1 = read(fd, tmp_buf, count);
			printf
			    ("TEST 6 In Read while loop Count is %d Count is: %d\n",
			     count1, count);

			if (count1 < 0) {
				printf
				    ("TEST 6 Read FAILED on channel %s iteratiOn %d\n",
				     IPC_CHANNEL3, i);
				break;
			}
			count -= count1;
			tmp_buf += count1;
		}
		result = check_data_integrity(wbuf, rbuf, count);
		if (result == -1) {
			printf("TEST 6 FAILED on channel %s iteration %d\n",
			       IPC_CHANNEL3, i);
			break;
		}
	}

	close(fd);

	printf("TEST 6 OK\n");
}

void test7(void)
{
	ssize_t bytes_written;
	ssize_t bytes_read;
	int result = 0;
	int fd;
	int iovcnt;
	struct iovec iov[3];

	char *buf0 = "short string\n";
	char *buf1 = "This is a longer string\n";
	char *buf2 = "This is the longest string in this example\n";

	iov[0].iov_base = buf0;
	iov[0].iov_len = strlen(buf0);

	iov[1].iov_base = buf1;
	iov[1].iov_len = strlen(buf1);

	iov[2].iov_base = buf2;
	iov[2].iov_len = strlen(buf2);

	fd = open(IPC_CHANNEL3, O_RDWR);
	if (fd == -1) {
		printf("TEST 7 failed with error %d\n", errno);
		return;
	}

	strcat(wbuf, buf0);
	strcat(wbuf, buf1);
	strcat(wbuf, buf0);

	iovcnt = sizeof(iov) / sizeof(struct iovec);
	bytes_written = writev(fd, iov, iovcnt);

	bytes_read = read(fd, rbuf, bytes_written);
	if (bytes_read < 0) {
		printf("TEST 7 Read FAILED on channel %s\n", IPC_CHANNEL3);
		goto out7;
	}

	result = check_data_integrity(wbuf, rbuf, bytes_read);
	if (result == -1) {
		printf("TEST 7 FAILED on channel %s\n", IPC_CHANNEL3);
	}

      out7:
	close(fd);

	printf("TEST 7 OK\n");

}

void test8(void)
{
	int count = 0, count1 = 0, result, iterations;
	int fd = -1;
	int i;
	struct pollfd fds;
	int timeout = 2000;
	int retval;
	char *tmp_buf;

	printf("TEST 8. Open IPC channel 3 and send data\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(IPC_CHANNEL3, O_RDWR | O_NONBLOCK);
	if (fd == -1) {
		printf("TEST 8 failed with error %d\n", errno);
		return;
	}

	iterations = 10;

	for (i = 0; i < iterations; i++) {
		fill_buffer(wbuf, TRANFER_SIZE);

		count = write(fd, wbuf, TRANFER_SIZE);
		if (count < 0) {
			printf
			    ("TEST 8 Write FAILED on channel %s iteration %d\n",
			     IPC_CHANNEL3, i);
			break;
		}
		tmp_buf = rbuf;
		while (count > 0) {
			fds.fd = fd;
			fds.events = POLLIN;
			fds.revents = 0;
			retval = poll(&fds, 1, timeout);
			if (retval == 0) {
				printf
				    ("Test 8 FAILED: Poll timeout, no data to read from BP.\n");
				return;
			}

			count1 = read(fd, tmp_buf, count);
			if (count1 < 0) {
				printf
				    ("TEST 8 Read FAILED on channel %s iteratiOn %d\n",
				     IPC_CHANNEL3, i);
				return;
			}
			count -= count1;
			tmp_buf += count1;
		}
		result = check_data_integrity(wbuf, rbuf, count);
		if (result == -1) {
			printf("TEST 8 FAILED on channel %s iteration %d\n",
			       IPC_CHANNEL3, i);
			break;
		}
	}

	close(fd);

	printf("TEST 8 OK\n");
}

void test20(void)
{
	struct ioctl_args args;
	int result;
	int fd = -1;

	printf("TEST 20. Open the IPC TEST driver and issue an open/close\n");
	printf("sequence on IPC channel 0 & 1 (short message)\n\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(DEVICE_NAME, O_RDWR);
	if (fd == -1) {
		goto error;
	}

	memset(&args, 0, sizeof(struct ioctl_args));
	result = ioctl(fd, 0x0, (unsigned long)&args);
	if (result == -1) {
		goto error;
	}

	args.channel = 1;
	result = ioctl(fd, 0x0, (unsigned long)&args);
	if (result == -1) {
		goto error;
	}

	args.channel = 2;
	result = ioctl(fd, 0x0, (unsigned long)&args);
	if (result == -1) {
		goto error;
	}

	args.channel = 3;
	result = ioctl(fd, 0x0, (unsigned long)&args);
	if (result == -1) {
		goto error;
	}

	printf("TEST 20 OK\n");

	close(fd);
	return;

      error:
	printf("TEST 20 FAILED\n");
	close(fd);
}

void test21(void)
{
	struct ioctl_args args;
	int result;
	int fd = -1;

	memset(&args, 0, sizeof(struct ioctl_args));
	args.bytes = 512;
	args.iterations = 10;
	/*vchannel no longer used, as now the IPC API internally assigne a virtual channel number */
	args.vchannel = 0;

	printf("TEST 21. Open the IPC TEST driver and send %d bytes\n",
	       args.bytes);
	printf("on IPC channel 3 (Packet Data)\n\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(DEVICE_NAME, O_RDWR);
	if (fd == -1) {
		goto error;
	}

	result = ioctl(fd, 0x1, (unsigned long)&args);
	if (result == -1) {
		goto error;
	}

	printf("TEST 21 OK\n");

	close(fd);
	return;

      error:
	printf("TEST 21 FAILED\n");
	close(fd);
}

void test22(void)
{
	struct ioctl_args args;
	int result;
	int fd = -1;

	memset(&args, 0, sizeof(struct ioctl_args));
	args.iterations = 10;
	args.channel = 3;
	/*vchannel no longer used, as now the IPC API internally assigne a virtual channel number */
	args.vchannel = 30;

	printf("TEST 22. Open the IPC TEST driver and send %d bytes\n",
	       args.bytes);
	printf("on IPC channel 2 (Short Message)\n\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(DEVICE_NAME, O_RDWR);
	if (fd == -1) {
		goto error;
	}

	result = ioctl(fd, 0x2, (unsigned long)&args);
	if (result == -1) {
		goto error;
	}

	printf("TEST 22 OK\n");

	close(fd);
	return;

      error:
	printf("TEST 22 FAILED\n");
	close(fd);
}

void test23(void)
{
	struct ioctl_args args;
	int result;
	int fd = -1;

	memset(&args, 0, sizeof(struct ioctl_args));
	args.iterations = 10;
	args.channel = 0;
	/*vchannel no longer used, as now the IPC API internally assigne a virtual channel number */
	args.vchannel = 31;

	printf("TEST 23. Open the IPC TEST driver and send %d bytes\n",
	       args.bytes);
	printf("on IPC channel 0 (Short Message)\n\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(DEVICE_NAME, O_RDWR);
	if (fd == -1) {
		goto error;
	}

	result = ioctl(fd, 0x2, (unsigned long)&args);
	if (result == -1) {
		goto error;
	}

	printf("TEST 23 OK\n");

	close(fd);
	return;

      error:
	printf("TEST 23 FAILED\n");
	close(fd);
}

void test24(void)
{
	struct ioctl_args args;
	int result;
	int fd = -1;

	memset(&args, 0, sizeof(struct ioctl_args));
	args.bytes = 512;
	args.iterations = 10;
	/*vchannel no longer used, as now the IPC API internally assigne a virtual channel number */
	args.vchannel = 0;

	printf("TEST 24. Open the IPC TEST driver and send %d bytes\n",
	       args.bytes);
	printf("on IPC channel 3 (Packet Data)\n\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(DEVICE_NAME, O_RDWR);
	if (fd == -1) {
		goto error;
	}

	result = ioctl(fd, 0x5, (unsigned long)&args);
	if (result == -1) {
		goto error;
	}

	printf("TEST 24 OK\n");

	close(fd);
	return;

      error:
	printf("TEST 24 FAILED\n");
	close(fd);
}

void test25(void)
{
	struct ioctl_args args;
	int result;
	int fd = -1;

	memset(&args, 0, sizeof(struct ioctl_args));
	args.bytes = 512;
	args.iterations = 10;
	/*vchannel no longer used, as now the IPC API internally assigne a virtual channel number */
	args.vchannel = 0;

	printf("TEST 25. Open the IPC TEST driver and send %d bytes\n",
	       args.bytes);
	printf("on IPC channel 3 (Packet Data)\n\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(DEVICE_NAME, O_RDWR);
	if (fd == -1) {
		goto error;
	}

	result = ioctl(fd, 0x6, (unsigned long)&args);
	if (result == -1) {
		goto error;
	}

	printf("TEST 25 OK\n");

	close(fd);
	return;

      error:
	printf("TEST 25 FAILED\n");
	close(fd);
}

void test26(void)
{
	struct ioctl_args args;
	int result;
	int fd = -1;

	memset(&args, 0, sizeof(struct ioctl_args));
	args.bytes = 3000;
	args.iterations = 10;
	/*vchannel no longer used, as now the IPC API internally assigne a virtual channel number  */
	args.vchannel = 0;

	printf("TEST 26. Open the IPC TEST driver and send %d bytes\n",
	       args.bytes);
	printf("on IPC channel 2 (Packet Data)\n\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(DEVICE_NAME, O_RDWR);
	if (fd == -1) {
		goto error;
	}

	result = ioctl(fd, 0x7, (unsigned long)&args);
	printf("TEST 26. IPC TEST driver Ioct return results %d\n", result);
	if (result == -1) {
		goto error;
	}

	printf("TEST 26 OK\n");

	close(fd);
	return;

      error:
	printf("TEST 26 FAILED\n");
	close(fd);
}

void test27(void)
{
	struct ioctl_args args;
	int result;
	int fd = -1;

	memset(&args, 0, sizeof(struct ioctl_args));
	args.bytes = 4500;
	args.iterations = 10;
	/*vchannel no longer used, as now the IPC API internally assigne a virtual channel number */
	args.vchannel = 0;

	printf
	    ("TEST 27. Open the IPC TEST driver and send %d bytes %d Radom is:\n",
	     args.bytes, args.messages);
	printf
	    ("on IPC channel 1 (Fixed Packet Data IPCV2 Read Benchmarking )\n\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(DEVICE_NAME, O_RDWR);
	if (fd == -1) {
		goto error;
	}

	result = ioctl(fd, 0x8, (unsigned long)&args);
	if (result == -1) {
		goto error;
	}

	printf("TEST 27 OK\n");

	close(fd);
	return;

      error:
	printf("TEST 27 FAILED\n");
	close(fd);
}

void test28(void)
{
	struct ioctl_args args;
	int result;
	int fd = -1;

	memset(&args, 0, sizeof(struct ioctl_args));
	args.bytes = 15000;
	args.iterations = 10;
	/*vchannel no longer used, as now the IPC API internally assigne a virtual channel number */
	args.vchannel = 0;

	printf
	    ("TEST 28. Open the IPC TEST driver and send %d bytes, %d iterations \n",
	     args.bytes, args.iterations);
	printf("on IPC channel 1 (Random Packet Data)\n\n");
	printf("press any key to start test\n");
	getchar();

	fd = open(DEVICE_NAME, O_RDWR);
	if (fd == -1) {
		goto error;
	}

	result = ioctl(fd, 0x9, (unsigned long)&args);
	if (result == -1) {
		goto error;
	}

	printf("TEST 28 OK\n");

	close(fd);
	return;

      error:
	printf("TEST 28 FAILED\n");
	close(fd);
}

int main(int argc, char **argv)
{
	int ipc_type = 0;
	int opt;

	while ((opt = getopt(argc, argv, "t:")) != EOF) {
		switch (opt) {
		case 't':
			ipc_type = atoi(optarg);
			break;
		default:
			return 1;
		}
	}

	switch (ipc_type) {
	case 1:
		test1();
		break;
	case 2:
		test2();
		break;
	case 3:
		test3();
		break;
	case 4:
		test4();
		break;

	case 5:
		test5();
		break;

	case 6:
		test6();
		break;

	case 7:
		test7();
		break;

	case 8:
		test8();
		break;

	case 20:
		test20();
		break;

	case 21:
		test21();
		break;

	case 22:
		test22();
		break;

	case 23:
		test23();
		break;

	case 24:
		test24();
		break;

	case 25:
		test25();
		break;
	case 26:
		test26();
		break;
	case 27:
		test27();
		break;
	case 28:
		test28();
		break;
	default:
		break;
	}

	return 0;
}
