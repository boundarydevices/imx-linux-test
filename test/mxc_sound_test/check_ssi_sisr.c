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
#include <string.h>
#include <linux/soundcard.h>
#include "dbmx31-ctrls.h"

int main(int ac, char *av[])
{
	int fd_mixer;
	int err = 0;

	printf("Hi... \n");

	if (ac != 1) {
		printf
		    ("check_ssi_sisr <noarg>, check that SISR flags are updated correctly\n");
		return 0;
	}

	if ((fd_mixer = open("/dev/sound/mixer", O_RDWR)) < 0) {
		printf("Error opening /dev/mixer");
		err++;
		goto _err_drv_open;
	}

	if (ioctl(fd_mixer, SNDCTL_DBMX_HW_SSI_FIFO_FULL, 0) < 0)
		err++;

	close(fd_mixer);

	if (err)
		goto _err_drv_open;
	return 0;

	close(fd_mixer);
      _err_drv_open:
	printf("Encountered %d errors\n", err);
	return err;
}
