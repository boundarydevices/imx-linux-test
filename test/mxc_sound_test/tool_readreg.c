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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include "dbmx31-ctrls.h"

int main(int ac, char *av[])
{
	int fd_mixer;
	unsigned int reg;
	dbmx_cfg cfg;

	printf("Hi... \n");

	if (ac == 1) {
		printf
		    ("read <reg>\n Reads a register at the $BASE+offset address specified\n");
		return 0;
	}

	if ((fd_mixer = open("/dev/sound/mixer", O_WRONLY)) < 0) {
		printf("Error opening /dev/sound/mixer");
		return 0;
	}

	sscanf(av[1], "%x", &reg);
	cfg.reg = reg;

	if ((reg < 0x43f00000) && (reg > 0x44000000))
		printf("Bad address\n");
	else {
		ioctl(fd_mixer, SNDCTL_DBMX_HW_READ_REG, &cfg);
		printf("Read returned = %x\n", cfg.val);
	}

	close(fd_mixer);

	return 0;
}
