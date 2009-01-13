/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All rights reserved.
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
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define BUF_MAX_SIZE	0x1000
#define DEV_SPI1	"/dev/spidev1.4"
#define DEV_SPI2   	"/dev/spidev2.4"
#define DEV_SPI3   	"/dev/spidev3.4"
static unsigned char mode;
static unsigned char bits_per_word = 8;
static unsigned int speed = 100000;

char *buffer;

void help_info(const char *appname)
{
	printf("\n"
	       "*******************************************************\n"
	       "*******    Test Application for SPI driver    *********\n"
	       "*******************************************************\n"
	       "*    This test sends bytes of the last parameter to   *\n"
	       "*    a specific SPI device. The maximum transfer      *\n"
	       "*    bytes are 4096 bytes for bits per word less than *\n"
	       "*    8(including 8), 2048 bytes for bits per word     *\n"
	       "*    between 9 and 16, 1024 bytes for bits per word   *\n"
	       "*    larger than 17(including 17). SPI writes data    *\n"
	       "*    received data from the user into Tx FIFO and     *\n"
	       "*    waits for the data in the Rx FIFO. Once the data *\n"
	       "*    is ready in the Rx FIFO, it is read and sent to  *\n"
	       "*    user. Test is considered successful if data      *\n"
	       "*    received = data sent.                            *\n"
	       "*                                                     *\n"
	       "*    NOTE: As this test is intended to test the SPI   *\n"
	       "*    device,it is always configured in loopback mode  *\n"
	       "*    reasons is that it is dangerous to send ramdom   *\n"
	       "*    data to SPI slave devices.                       *\n"
	       "*                                                     *\n"
	       "*    Options : %s                                      \n"
	       "                  [-D spi_no]  [-s speed]              \n"
	       "                  [-b bits_per_word]                   \n"
	       "                  [-H] [-O] [-C] <value>               \n"
	       "*                                                     *\n"
	       "*    <spi_no> - CSPI Module number in [0, 1, 2]       *\n"
	       "*    <speed> - Max transfer speed                     *\n"
	       "*    <bits_per_word> - bits per word                  *\n"
	       "*    -H - Phase 1 operation of clock                  *\n"
	       "*    -O - Active low polarity of clock                *\n"
	       "*    -C - Active high for chip select                 *\n"
	       "*    <value> - Actual values to be sent               *\n"
	       "*******************************************************\n"
	       "\n", appname);
}

int check_data_integrity(char *buf1, char *buf2, int count)
{
	int result = 0;
	int i;
	short *tmp1_s = NULL, *tmp2_s = NULL;
	int *tmp1_i = NULL, *tmp2_i = NULL;
	int mask;

	if (bits_per_word > 16) {
		tmp1_i = (int *)buf1;
		tmp2_i = (int *)buf2;
		count /= 4;
	} else if (bits_per_word > 8) {
		tmp1_s = (short *)buf1;
		tmp2_s = (short *)buf2;
		count /= 2;
	}
	mask = (1 << bits_per_word) - 1;

	for (i = 0; i < count; i++) {
		if (bits_per_word > 16) {
			if ((tmp1_i[i] & mask) != tmp2_i[i]) {
				printf("Corrupted data at %d wbuf = %d rbuf = %d\n",
			       i, tmp1_i[i], tmp2_i[i]);
				result = -1;
			}
		} else if (bits_per_word > 8) {
			if ((tmp1_s[i] & (short) mask) != tmp2_s[i]) {
				printf("Corrupted data at %d wbuf = %d rbuf = %d\n",
			       i, tmp1_s[i], tmp2_s[i]);
				result = -1;
			}
		} else
			if ((buf1[i] & (char) mask) != buf2[i]) {
				printf("Corrupted data at %d wbuf = %d rbuf = %d\n",
			       i, buf1[i] & (char) mask, buf2[i]);
				result = -1;
			}
	}

	return result;
}

static int transfer(int fd, char *tbuf, char *rbuf, int bytes)
{
	int ret;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tbuf,
		.rx_buf = (unsigned long)rbuf,
		.len = bytes,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret == 1)
		printf("can't send spi message");
	return ret;
}

int execute_buffer_test(int spi_id, int len, char *buffer)
{
	char *rbuf;
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
		printf("Error:cannot open device "
		       "(Maybe not present in your board?)\n");
		return -1;
	}

	res = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (res == -1) {
		printf("can't set spi mode");
		goto exit;
	}

	res = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (res == -1) {
		printf("can't set spi mode");
		goto exit;
	}
	/*
	 * bits per word
	 */
	res = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word);
	if (res == -1) {
		printf("can't set bits per word");
		goto exit;
	}

	res = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits_per_word);
	if (res == -1) {
		printf("can't get bits per word");
		goto exit;
	}

	/*
	 * max speed hz
	 */
	res = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (res == -1) {
		printf("can't set max speed hz");
		goto exit;
	}

	res = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (res == -1) {
		printf("can't get max speed hz");
		goto exit;
	}

	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits_per_word);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	rbuf = malloc(len);
	memset(rbuf, 0, len);

	res = transfer(fd, buffer, rbuf, len);
	if (res < 0) {
		printf("Failed transferring data: %d\n", errno);
		return -1;
	}

	res = check_data_integrity(buffer, rbuf, len);
	printf("Data sent : %s\n", (char *)buffer);
	printf("Data received : %s\n", (char *)rbuf);

	if (res != 0) {
		printf("Test FAILED.\n");
	} else {
		printf("Test PASSED.\n");
	}
	free(rbuf);
exit:
	close(fd);

	return 0;
}

int main(int argc, char **argv)
{
	int spi_id;
	int bytes, len;
	int res;
	int i;

	if (argc <= 1) {
		help_info(argv[0]);
		return 1;
	}

	mode = 0;
	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-D")) {
			i++;
			spi_id = atoi(argv[i]);
			if (spi_id < 0 || spi_id > 2) {
				printf("invalid parameter for device option\n");
				help_info(argv[0]);
				return -1;
			}
		} else if(!strcmp(argv[i], "-s")) {
			i++;
			speed = atoi(argv[i]);
		} else if(!strcmp(argv[i], "-b")) {
			i++;
			bits_per_word = atoi(argv[i]);
		} else if(!strcmp(argv[i], "-H"))
			mode |= SPI_CPHA;
		else if(!strcmp(argv[i], "-O"))
			mode |= SPI_CPOL;
		else if(!strcmp(argv[i], "-C"))
			mode |= SPI_CS_HIGH;
		else if((i != (argc - 1))) {
			printf("invalid parameter\n");
			help_info(argv[0]);
			return -1;
		}
	}

	bytes = strlen((char *)argv[argc - 1]);
	if (bytes < 1) {
		printf("invalid parameter for buffer size\n");
		help_info(argv[0]);
		return -1;
	}

	/* The spidev driver copies len of bytes to tx buffer, and sends len of
	   units to SPI device. So it will send much more data to the SPI
	   device when bits per word is larger than 8, we only care the
	   first len of bytes for this test */
	len = bytes;
	if (bits_per_word <= 8 && bytes > BUF_MAX_SIZE)
		len = BUF_MAX_SIZE;
	else if (bits_per_word <= 16 && bytes > BUF_MAX_SIZE/2)
		len = BUF_MAX_SIZE/2;
	else if (bytes > BUF_MAX_SIZE/4)
		len = BUF_MAX_SIZE/4;

	buffer = malloc(BUF_MAX_SIZE);
	memset(buffer, 0, BUF_MAX_SIZE);
	strcpy(buffer, (char *)argv[argc-1]);

	printf("Execute data transfer test: %d %d %s\n", spi_id, len, buffer);
	res = execute_buffer_test(spi_id, len, buffer);
	free(buffer);
	return res;
}
