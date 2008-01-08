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

#define SSI_REG_NB 			16
#define SSI_REG_BASE			0x43fa0000
#define LABELS {"SCR", "SISR", "SIER", "STCR", "SRCR", "STCCR", "SRCCR", "SRCSR", \
		"STR", "SOR", "SACNT", "SACADD", "SACDAT", "SATAG", "STMSK", "SRMSK"}

int main(int ac, char *av[])
{
	int fd_mixer, i;
	dbmx_cfg str;
	const char *labels[] = LABELS;

	if (ac != 1) {
		printf("main\n Dumps the SSI registers\n");
		return 0;
	}

	if ((fd_mixer = open("/dev/sound/mixer", O_RDWR)) < 0) {
		printf("Error opening /dev/sound/mixer");
		return 0;
	}

	for (i = 0; i < SSI_REG_NB; i++) {
		str.reg = SSI_REG_BASE + 4 * i + 0x10;
		ioctl(fd_mixer, SNDCTL_DBMX_HW_SSI_CFG_R, &str);
		printf("Register %s = %x\n", labels[i], str.val);
	}

	close(fd_mixer);
	return 0;
}
