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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "mxc_test.h"

#define	_A(a)		"wdog_test: "a"\n"

static void HelpInfo()
{
	printf(_A("Usage: wdog_test <number>. Need to reset after each test"));
	printf(_A("  0 : Enable/service both WDOGs - normal operation"));
	printf(_A("  1 : Stuck in ISR (WDOGs enabled) - reboot"));
	printf(_A("  2 : WDOG1 not serviced (both WDOGs enabled) - reboot\n"));
	printf(_A("  --  these two tests only apply to certain platforms --"));
	printf(_A("  3 : WDOG2 enabled & not serviced - infinite messsages"));
	printf(_A("  4 : Both enabled but 2 not serviced"));
	printf(_A("      - messages from 2nd then reboot as 1 not serviced "));
}

int main(int argc, char **argv)
{
	int fd, ret;
	int param;

	if (argc < 2) {
		HelpInfo();
		return 1;
	}

	param = argv[1][0] - '0';

	printf(_A("Starting WDOG test case: %d\n"), param);

	fd = open("/dev/mxc_wdog_tm", O_RDWR);
	if (fd < 0) {
		printf("Open failed with err %d\n", fd);
		perror("Open");
		return -1;
	}

	if ((ret = ioctl(fd, MXCTEST_WDOG, &param)) < 0) {
		printf("wdog ioctl call failed with err %d\n", ret);
		perror("ioctl");
		return -1;
	}

	return 0;
}
