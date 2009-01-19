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
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "soundcard.h"
#include "audio_controls.h"
 int main(int argc, char **argv)
{
	int fd_mixer = -1;
	int balance = 0;
	int tmp = 0;
	int res;
	int delay;
	printf("opening audio mixer\n");
	fd_mixer = open("/dev/sound/mixer", O_RDWR);
	if (fd_mixer < 0) {
		printf("Error opening /dev/sound/mixer");
		return 1;
	}
	delay = 1;
	while (tmp <= 240) {
		printf("Applying balance = %d\n", balance);
		res =
		    ioctl(fd_mixer, SNDCTL_MC13783_WRITE_OUT_BALANCE, &balance);
		if (res == -1) {
			printf
			    ("SNDCTL_MC13783_WRITE_OUT_BALANCE ioctl error: %d\n",
			     errno);
			return -1;
		}
		balance = (balance == 100) ? 0 : (balance + 5);
		printf("Sleeping for %d seconds\n", delay);
		sleep(delay);
		tmp += delay;
	}
	return 0;
}


