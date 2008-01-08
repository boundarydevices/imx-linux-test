/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All rights reserved.
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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BUF_MAX_SIZE	0x21	/* 32 + 1 to hold 'NULL' */
#define DEV_SPI1	"/dev/spi1"
#define DEV_SPI2   	"/dev/spi2"
#define DEV_SPI3   	"/dev/spi3"

char buffer[BUF_MAX_SIZE] = { 0 };

void help_info(const char *appname)
{
	printf("\n"
	       "*******************************************************\n"
	       "*******    Test Application for SPI driver    *********\n"
	       "*******************************************************\n"
	       "*    This test send up to 32 bytes to a specific SPI  *\n"
	       "*    device. SPI writes data received data from the   *\n"
	       "*    user into Tx FIFO and waits for the data in the  *\n"
	       "*    Rx FIFO. Once the data is ready in the Rx FIFO,  *\n"
	       "*    it is read and sent to user. Test is considered  *\n"
	       "*    successful if data received = data sent.         *\n"
	       "*                                                     *\n"
	       "*    NOTE: As this test is intended to test the SPI   *\n"
	       "*    device,it is always configured in loopback mode  *\n"
	       "*    reasons is that it is dangerous to send ramdom   *\n"
	       "*    data to SPI slave devices.                       *\n"
	       "*                                                     *\n"
	       "*    Options : %s                                      \n"
	       "                  <spi_no> <nb_bytes> <value>         *\n"
	       "*                                                     *\n"
	       "*    <spi_no> - CSPI Module number in [0, 1, 2]       *\n"
	       "*    <nb_bytes> - No. of bytes: [1-32]                *\n"
	       "*    <value> - Actual value to be sent                *\n"
	       "*******************************************************\n"
	       "\n", appname);
}

int check_data_integrity(char *buf1, char *buf2, int count)
{
	int result = 0;
	int i;

	for (i = 0; i < count; i++) {
		if (buf1[i] != buf2[i]) {
			printf("Corrupted data at %d wbuf = %d rbuf = %d\n",
			       i, buf1[i], buf2[i]);
			result = -1;
		}
	}

	return result;
}

int execute_buffer_test(int spi_id, int bytes, char *buffer)
{
	char rbuf[BUF_MAX_SIZE];
	int res = 0;
	int fd = -1;

	if (spi_id == 0) {
		fd = open(DEV_SPI1, O_RDWR);
	} else if (spi_id == 1) {
		fd = open(DEV_SPI2, O_RDWR);
	} else if (spi_id == 2) {
		fd = open(DEV_SPI3, O_RDWR);
	}

	if (fd < 0) {
		printf
		    ("Error: cannot open device (Maybe not present in your board?)\n");
		return -1;
	}
	/* write data */
	res = write(fd, buffer, bytes);
	if (res != bytes) {
		printf("Failed when writing data to SPI: %d %d\n", errno, res);
		return -1;
	}

	memset(rbuf, 0, BUF_MAX_SIZE);

	/* read data back */
	res = read(fd, rbuf, bytes);
	if (res != bytes) {
		printf("Failed when reading data from SPI: %d\n", errno);
		return -1;
	}
	rbuf[BUF_MAX_SIZE] = '\0';

	res = check_data_integrity(buffer, rbuf, bytes);
	printf("Data sent : %s\n", (char *)buffer);
	printf("Data received : %s\n", (char *)rbuf);

	if (res != 0) {
		printf("Test FAILED.\n");
	} else {
		printf("Test PASSED.\n");
	}

	close(fd);

	return 0;
}

int main(int argc, char **argv)
{
	int spi_id;
	int bytes;
	int res;

	if (argc != 4) {
		help_info(argv[0]);
		return 1;
	}

	spi_id = atoi(argv[1]);
	if (spi_id < 0 || spi_id > 2) {
		printf("invalid parameter for device option\n");
		help_info(argv[0]);
		return -1;
	}

	bytes = atoi(argv[2]);
	if (bytes > BUF_MAX_SIZE || bytes < 1) {
		printf("invalid parameter for buffer size\n");
		help_info(argv[0]);
		return -1;
	}

	memset(buffer, 0, BUF_MAX_SIZE);

	if (strncmp((char *)argv[3], "0x", 2)) {
		strcpy(buffer, (char *)argv[3]);
	} else {
		strcpy(buffer, (char *)argv[3] + 2);
	}

	buffer[BUF_MAX_SIZE] = '\0';
	printf("Execute data transfer test: %d %d %s\n", spi_id, bytes, buffer);
	res = execute_buffer_test(spi_id, bytes, buffer);

	return res;
}
