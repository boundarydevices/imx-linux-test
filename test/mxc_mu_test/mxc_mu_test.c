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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include "../include/mxc_test.h"

int main(int argc, char **argv)
{
	int fp[4], bytes_written = 0, i, j, bytes_read = 0;
	int flush;
	unsigned long M2RAM_Phy_Addr;
	unsigned int platform;
	struct pollfd fds;
	unsigned int ret;
	int write_status = 0, read_status = 0, poll_read = 0, poll_write = 0;

	unsigned int writereq = 0x00050001;
	unsigned int readreq = 0x00050002;
	unsigned int address = 0xff002800;
	printf("================== TESTING MU DRIVER ==================\n");
	printf
	    ("Please enter M2RAM Physical address for this platform e.g ff000000 \n");
	scanf("%x", &M2RAM_Phy_Addr);
	address = M2RAM_Phy_Addr;
	/*
	   printf("Please specify Hardware platform..... \n");
	   printf(" 1 = mxc27520evb \n");
	   printf(" 2 = mxc27530evb \n");
	   printf(" 3 = mxc30030\n");
	   printf(" 4 = mxc30020evb \n");
	   printf(" 5 = mxc30030evb \n");
	   printf(" 6 = mxc91131evb \n" );
	   scanf("%d", &platform);
	   printf("Platform value in application is: %x \n", platform);
	 */
	unsigned int datavalue1 = 0xaaaaaaa1;
	unsigned int datavalue2 = 0xbbbbbbb2;
	unsigned int datavalue3 = 0xccccccc3;
	unsigned int datavalue4 = 0xddddddd4;
	unsigned int datavalue5 = 0xeeeeeee5;
	unsigned char writecode[4], addr[4], data1[4], data2[4];
	unsigned char data3[4], data4[4], data5[4], readcode[4];
	unsigned char user_buff[1000];
	memset(user_buff, '\0', 1000);
	for (j = 3; j >= 0; j--) {
		writecode[j] = writereq >> (j * 8);
	}
	for (j = 3; j >= 0; j--) {
		readcode[j] = readreq >> (j * 8);
	}
	for (j = 3; j >= 0; j--) {
		addr[j] = address >> (j * 8);
	}
	for (j = 3; j >= 0; j--) {
		data1[j] = datavalue1 >> (j * 8);
	}
	for (j = 3; j >= 0; j--) {
		data2[j] = datavalue2 >> (j * 8);
	}
	for (j = 3; j >= 0; j--) {
		data3[j] = datavalue3 >> (j * 8);
	}
	for (j = 3; j >= 0; j--) {
		data4[j] = datavalue4 >> (j * 8);
	}
	for (j = 3; j >= 0; j--) {
		data5[j] = datavalue5 >> (j * 8);
	}

	printf("===================== OPEN Device =====================\n");
	fp[0] = open("/dev/mxc_mu/0", O_RDWR | O_NONBLOCK);
	fp[1] = open("/dev/mxc_mu/1", O_RDWR | O_NONBLOCK);
	fp[2] = open("/dev/mxc_mu/2", O_RDWR | O_NONBLOCK);
	fp[3] = open("/dev/mxc_mu/3", O_RDWR | O_NONBLOCK);
	/* Enabling Byte swapping on read */
	ioctl(fp[0], 0x8);
	ioctl(fp[1], 0x8);
	ioctl(fp[2], 0x8);
	ioctl(fp[3], 0x8);

	if (fp[0] < 0 || fp[1] < 0 || fp[2] < 0 || fp[3] < 0) {
		printf("Open failed with err %d\n", fp[0]);
		perror("Open");
		return -1;
	} else {
		printf("Open Device is %d\n", fp[0]);
		/*Need to determine why dummy read is required */
		/*the test works without it */
		//if((platform==mxc91231) || (platform==mxc91311)){
		//printf("Doing a dummy read\n");
		//read(fp[0], user_buff, 4);
		//memset(user_buff, '\0', 1000);
		//      }
		printf
		    ("================== After OPEN Device =================\n");
		printf
		    ("=============== Before Calling Write =================\n");
		printf("Now calling 1st write\n");
		bytes_written = write(fp[0], writecode, 4);	//0x12345678 = 4 bytes                                                                                   //each digit pos = 4 binary bits
		printf("Total Bytes actually written are %d\n", bytes_written);
		printf("Now calling 2nd write\n");
		bytes_written = write(fp[1], addr, 4);
		printf("Total Bytes actually written are %d\n", bytes_written);
		printf("Now calling data writes\n");
		bytes_written = write(fp[2], data1, 4);
		bytes_written = write(fp[3], data2, 4);
		bytes_written = write(fp[0], data3, 4);
		bytes_written = write(fp[1], data4, 4);
		bytes_written = write(fp[2], data5, 4);
		/* Read write response */
		bytes_read = read(fp[0], user_buff, 4);
		for (i = 0; i < bytes_read; i++) {
			printf("Read Buffer after write response is %x\n",
			       user_buff[i]);
		}
		if ((user_buff[0] == 0x00) && (user_buff[1] == 0x05)
		    && (user_buff[2] == 0x00) && (user_buff[3] == 0x01)) {
			write_status = 1;
		}
		//printf("Press key when ready... \n");
		//scanf("%d",&i);
		/* Sending Read Request */
		bytes_written = write(fp[0], readcode, 4);
		bytes_written = write(fp[1], addr, 4);
		printf
		    ("================ Before Calling all Reads  =================\n");
		bytes_read = read(fp[1], user_buff, 4);
		for (i = 0; i < bytes_read; i++) {
			printf("Read Buffer after read 1 from MRR1 is %x\n",
			       user_buff[i]);
		}
		bytes_read = read(fp[2], user_buff, 4);
		for (i = 0; i < bytes_read; i++) {
			printf("Read Buffer after read 2 from MRR2 is %x\n",
			       user_buff[i]);
		}
		bytes_read = read(fp[3], user_buff, 4);
		for (i = 0; i < bytes_read; i++) {
			printf("Read Buffer after read 3 from MRR3 is %x\n",
			       user_buff[i]);
		}
		bytes_read = read(fp[0], user_buff, 4);
		bytes_read = read(fp[0], user_buff, 4);
		for (i = 0; i < bytes_read; i++) {
			printf("Read Buffer after read 4 from MRR0 is %x\n",
			       user_buff[i]);
		}
		bytes_read = read(fp[1], user_buff, 4);
		for (i = 0; i < bytes_read; i++) {
			printf("Read Buffer after read 5 from MRR1 is %x\n",
			       user_buff[i]);
		}
		/* Read read response */
		bytes_read = read(fp[0], user_buff, 4);
		for (i = 0; i < bytes_read; i++) {
			printf("Read Buffer after read response is %x\n",
			       user_buff[i]);
		}
		if ((user_buff[0] == 0x00) && (user_buff[1] == 0x05)
		    && (user_buff[2] == 0x00) && (user_buff[3] == 0x02)) {
			read_status = 1;
		}
		printf
		    ("\n ####### TESTING POLLING FEATURE on MU channel 3 #######\n");
		printf
		    ("\nNow that all the data are transmitted and received, \n");
		printf
		    ("a POLL on a particular channel should indicate no data\n");
		printf
		    ("to be read and more data can be transmitted i.e., buffer empty \n\n");
		fds.fd = fp[3];
		fds.events = POLLIN;
		ret = poll(&fds, 1, 0);
		if (ret == 0) {
			printf
			    ("READ: POLLIN - Timeout, No data to read using channel 3.\n\n");
			poll_read = 1;
		} else {
			bytes_read = read(fp[3], user_buff, 4);
			for (i = 0; i < bytes_read; i++) {
				printf
				    ("Read Buffer after read 3 from MRR3 is %x\n\n",
				     user_buff[i]);
			}
		}
		/* Now change the events to WRITE */
		fds.events = POLLOUT;
		ret = poll(&fds, 1, 0);
		if (ret == 0) {
			printf
			    ("WRITE: POLLOUT - Timeout, No space to write.using channel 3\n\n");
		} else {
			printf
			    ("WRITE: POLLOUT - Space Available in write buffer.\n");
			printf
			    ("So data can be transmitted using channel 3\n\n");
			poll_write = 1;
		}
		printf("Now closing all the channels\n\n");
		flush = close(fp[0]);
		if (flush < 0) {
			printf("Close failed with err %d\n", flush);
			perror("Close");
			return -1;
		}
		flush = close(fp[1]);
		if (flush < 0) {
			printf("Close failed with err %d\n", flush);
			perror("Close");
			return -1;
		}
		flush = close(fp[2]);
		if (flush < 0) {
			printf("Close failed with err %d\n", flush);
			perror("Close");
			return -1;
		}
		flush = close(fp[3]);
		if (flush < 0) {
			printf("Close failed with err %d\n", flush);
			perror("Close");
			return -1;
		}
	}
	if ((write_status > 0) && (read_status > 0) && (poll_write > 0)
	    && (poll_read > 0)) {
		printf("MU Test PASSED\n\n");
		return 0;
	} else {
		return -1;
	}
}
