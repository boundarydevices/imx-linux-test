/*
 * Copyright 2008 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */


#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>

#define I2C_SLAVE_FORCE 0x0706

#define I2C_MASTER_DEV "/dev/i2c-0"
#define I2C_SLAVE_ADDR 0x55
#define I2C_SLAVE_DEV "/dev/slave-i2c-0"

#define I2C_SLAVE_TEST_DATA 0x55
#define I2C_SLAVE_TEST_NUM 100

static char master_buffer[I2C_SLAVE_TEST_NUM];
static char slave_buffer[I2C_SLAVE_TEST_NUM];

static void *slave_thread(void *data)
{
	int fd;
	int ret, i;

	fd = open(I2C_SLAVE_DEV, O_RDWR);
	if (fd < 0) {
		printf("i2c-slave-test:open device error\n");
		printf("i2c-slave-test: slave thread: Fail\n");
		return 0;
	}

	memset(slave_buffer, I2C_SLAVE_TEST_DATA, I2C_SLAVE_TEST_NUM);

	printf
	    ("i2c-slave-test: slave thread: about to send %d bytes, value=0x%x\n",
	     I2C_SLAVE_TEST_NUM, I2C_SLAVE_TEST_DATA);

	ret = write(fd, slave_buffer, I2C_SLAVE_TEST_NUM);

	if (ret < 0) {
		printf("i2c-slave-test: slave thread: send data error\n");
		goto fail;
	} else {
		printf
		    ("i2c-slave-test: slave thread: send %d bytes, while %d wanted\n",
		     ret, I2C_SLAVE_TEST_NUM);
	}

	memset(slave_buffer, 0, I2C_SLAVE_TEST_NUM);
	printf
	    ("i2c-slave-test: slave thread: about to receive %d bytes, value=0x%x\n",
	     I2C_SLAVE_TEST_NUM, I2C_SLAVE_TEST_DATA);

	ret = read(fd, slave_buffer, I2C_SLAVE_TEST_NUM);
	if (ret < 0) {
		printf("i2c-slave-test: slave thread: receive data error\n");
		goto fail;
	} else {
		for (i = 0; i < I2C_SLAVE_TEST_NUM; i++) {
			if (slave_buffer[i] != I2C_SLAVE_TEST_DATA){
				printf
				    ("i2c-slave-test: slave thread: received data wrong!\n");
			    goto fail;
            }
		}

		printf
		    ("i2c-slave-test: slave thread: reveive %d bytes while %d wanted\n",
		     ret, I2C_SLAVE_TEST_NUM);
	}

	printf("i2c-slave-test: slave thread: Success\n");
	close(fd);
	return 0;

      fail:
	printf("i2c-slave-test: slave thread: Fail\n");
	close(fd);

	return 0;
}

int main(int argc, char *argv[])
{
	int fd;
	int ret, i;
	pthread_t slave_thread_id;

	fd = open(I2C_MASTER_DEV, O_RDWR);
	if (fd < 0) {
		printf("i2c-slave-test:master thread:open device error\n");
		goto fail;
	}

	ret = ioctl(fd, I2C_SLAVE_FORCE, I2C_SLAVE_ADDR);
	if (ret < 0) {
		printf("i2c-slave-test:master thread:ioctl error\n");
		goto fail;
	}

	pthread_create(&slave_thread_id, 0, slave_thread, (void *)0);

	usleep(1000*100);

	/*reveive data test */
	printf("i2c-slave-test:master thread:about to receive %d bytes\n",
	       I2C_SLAVE_TEST_NUM);
	ret = read(fd, master_buffer, I2C_SLAVE_TEST_NUM);
	if (ret < 0) {
		printf("i2c-slave-test:master thread: read error\n");
		goto fail;
	} else {
		for (i = 0; i < I2C_SLAVE_TEST_NUM; i++) {
			if (master_buffer[i] != I2C_SLAVE_TEST_DATA) {
				printf
				    ("i2c-slave-test:master thread:received data error\n");
				goto fail;
			}
		}

		printf
		    ("i2c-slave-test:master thread:%d bytes received, while %d bytes wanted\n",
		     I2C_SLAVE_TEST_NUM, I2C_SLAVE_TEST_NUM);
	}

	/*send data test */
	printf("i2c-slave-test:master thread:about to send %d bytes\n",
	       I2C_SLAVE_TEST_NUM);
	memset(master_buffer, I2C_SLAVE_TEST_DATA, I2C_SLAVE_TEST_NUM);
	ret = write(fd, master_buffer, I2C_SLAVE_TEST_NUM);
	if (ret < 0) {
		printf("i2c-slave-test:master thread: send error\n");
		goto fail;
	} else {
		printf
		    ("i2c-slave-test:master thread:%d bytes sent, while %d bytes wanted\n",
		     I2C_SLAVE_TEST_NUM, I2C_SLAVE_TEST_NUM);
	}

	printf("i2c-slave-test:master thread: Success\n");
	close(fd);

	/*wait slave thread to finish. */
	usleep(1000*100);
	return 0;

      fail:
	printf("i2c-slave-test:master thread: Fail\n");
	close(fd);

	return -1;
}
