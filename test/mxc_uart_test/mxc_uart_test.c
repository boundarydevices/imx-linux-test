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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define LOOPBACK        0x8000

int main(int argc, char **argv)
{
        int uart_file1;
        unsigned int line_val;
        char buf[5];
        struct termios mxc, old;
        int retval;

        printf("Test: MXC UART!\n");
        printf("Usage: mxc_uart_test <UART device name, opens UART2 if no dev name is specified>\n");

        if (argc == 1) {
                /* No Args, open UART 2 */
                if ((uart_file1 = open("/dev/ttymxc/1", O_RDWR)) == -1) {
                        printf("Error opening UART 2\n");
                        exit(1);
                } else {
                        printf("Test: UART 2 opened\n");
                }
        } else {
                /* Open the specified UART device */
                if ((uart_file1 = open(*++argv, O_RDWR)) == -1) {
                        printf("Error opening %s\n", *argv);
                        exit(1);
                } else {
                        printf("%s opened\n", *argv);
                }
        }

        tcgetattr(uart_file1, &old);
        mxc = old;
        mxc.c_lflag &= ~(ICANON | ECHO | ISIG);
        retval = tcsetattr(uart_file1, TCSANOW, &mxc);
        printf("Attributes set\n");

        line_val = LOOPBACK;
        ioctl(uart_file1, TIOCMSET, &line_val);
        printf("Test: IOCTL Set\n");

        write(uart_file1, "Test\0", 5);
        printf("Data Written= Test\n");

        read(uart_file1, buf, 5);
        printf("Data Read back= %s\n", buf);
        sleep(2);
        ioctl(uart_file1, TIOCMBIC, &line_val);

        retval = tcsetattr(uart_file1, TCSAFLUSH, &old);

        close(uart_file1);
        return 0;
}
