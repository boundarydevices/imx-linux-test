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
        char buf[2];
        int fail = 0;

        printf("\nTest: I2C Test With mt9v111 camera!\n");

        i2c_file = open("/dev/mxc_test", O_RDWR);
        if (i2c_file < 0) {
                printf("Open failed\n");
                exit(-1);
        }

        /* Enable the CSI clock */
        ioctl(i2c_file, MXCTEST_I2C_CSICLKENB, &i2c_test);

        i2c_test.bus = 0;
        i2c_test.slave_addr = 0x48;

        printf("Slave address=0x%x\n\n", i2c_test.slave_addr);

        reg[0] = 0x01;
        buf[1] = 0x4;
        buf[0] = 0x0;
        i2c_test.reg = reg;
        i2c_test.reg_size = 1;
        i2c_test.buf = buf;
        i2c_test.buf_size = 2;

        printf("Data write: buf[0]=%x, buf[1]=%x to reg=%x\n", buf[0], buf[1], reg[0]);
        ioctl(i2c_file, MXCTEST_I2C_WRITE, &i2c_test);

        buf[0] = 0;
        buf[1] = 0;
        i2c_test.buf = buf;
        ioctl(i2c_file, MXCTEST_I2C_READ, &i2c_test);

        if (i2c_test.buf[0] != 0x0) {
                fail = 1;
        }

        if ((i2c_test.buf[1] & 0x4)!= 0x4) {
                fail = 1;
        }
        printf("Data read: buf[0]=%x, buf[1]=%x from reg=%x\n", buf[0], buf[1], reg[0]);

        if (fail == 1) {
                printf("\nI2C TEST FAILED\n\n");
        } else {
                printf("\nI2C TEST PASSED\n\n");
        }

        /* Disable the CSI clock */
        ioctl(i2c_file, MXCTEST_I2C_CSICLKDIS, &i2c_test);

        close(i2c_file);

        return 0;
}
