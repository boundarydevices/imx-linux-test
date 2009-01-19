/*
 * Copyright 2006-2009 Freescale Semiconductor, Inc. All rights reserved.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "mxc_test.h"

int main(int argc, char **argv)
{
        mxc_i2c_test i2c_test;
        int i2c_file;
        char reg[1];
        char buf[1];
        int fail = 0;

	printf("Test: I2C!\n");

        i2c_file = open("/dev/mxc_test", O_RDWR);
        if (i2c_file < 0) {
                printf("Open failed\n");
                exit(-1);
        }

        i2c_test.bus = 0;
        i2c_test.slave_addr = 0x2d; //USB Slave Address

        printf("Slave address=%x\n", i2c_test.slave_addr);

        reg[0] = 0x6;
        i2c_test.reg = reg;
        i2c_test.reg_size = 1;
        buf[0] = 0x20;
        i2c_test.buf = (char *)&buf;
        i2c_test.buf_size = 1;
        printf("Write Data=%x to reg=%x\n", buf[0], reg[0]);
        ioctl(i2c_file, MXCTEST_I2C_WRITE, &i2c_test);

        reg[0] = 0x4;
        i2c_test.reg = reg;
        i2c_test.reg_size = 1;
        buf[0] = 0x4;
        i2c_test.buf = (char *)&buf;
        i2c_test.buf_size = 1;
        printf("Write Data=%x to reg=%x\n", buf[0], reg[0]);
        ioctl(i2c_file, MXCTEST_I2C_WRITE, &i2c_test);

        reg[0] = 0x13;
        i2c_test.reg = reg;
        i2c_test.reg_size = 1;
        buf[0] = 0x4;
        i2c_test.buf = (char *)&buf;
        i2c_test.buf_size = 1;
        printf("Write Data=%x to reg=%x\n", buf[0], reg[0]);
        ioctl(i2c_file, MXCTEST_I2C_WRITE, &i2c_test);

        reg[0] = 0x12;
        i2c_test.reg = reg;
        i2c_test.reg_size = 1;
        buf[0] = 0x2;
        i2c_test.buf = (char *)&buf;
        i2c_test.buf_size = 1;
        printf("Write Data=%x to reg=%x\n", buf[0], reg[0]);
        ioctl(i2c_file, MXCTEST_I2C_WRITE, &i2c_test);

        reg[0] = 0x6;
        i2c_test.reg = reg;
        i2c_test.reg_size = 1;
        buf[0] = 0;
        i2c_test.buf = buf;
        i2c_test.buf_size = 1;
        ioctl(i2c_file, MXCTEST_I2C_READ, &i2c_test);
        if ((i2c_test.buf[0] & 0x20) != 0x20) {
                fail = 1;
        }
        printf("Data read=%x from reg=%x\n", buf[0], reg[0]);

        reg[0] = 0x4;
        i2c_test.reg = reg;
        i2c_test.reg_size = 1;
        buf[0] = 0;
        i2c_test.buf = buf;
        i2c_test.buf_size = 1;
        ioctl(i2c_file, MXCTEST_I2C_READ, &i2c_test);
        if ((i2c_test.buf[0] & 0x4) != 0x4) {
                fail = 1;
        }
        printf("Data read=%x from reg=%x\n", buf[0], reg[0]);

        reg[0] = 0x13;
        i2c_test.reg = reg;
        i2c_test.reg_size = 1;
        buf[0] = 0;
        i2c_test.buf = buf;
        i2c_test.buf_size = 1;
        ioctl(i2c_file, MXCTEST_I2C_READ, &i2c_test);
        if ((i2c_test.buf[0] & 0x2)!= 0x2) {
                fail = 1;
        }
        printf("Data read=%x from reg=%x\n", buf[0], reg[0]);

        if (fail == 1) {
                printf("\nI2C TEST FAILED\n\n");
        } else {
                printf("\nI2C TEST PASSED\n\n");
        }

        close(i2c_file);
	return 0;
}
