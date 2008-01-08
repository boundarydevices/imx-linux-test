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
#include <errno.h>
#include <sys/ioctl.h>

#include "soundcard.h"
#include "audio_controls.h"
 int main(int argc, char **argv) 
{
	int fd_mixer = -1;
	int left, right;
	int vol = 0;
	int tmp = 0;
	int res = 0;
	int down = 0;
	int delay;
	printf("opening audio mixer\n");
	fd_mixer = open("/dev/sound/mixer", O_RDWR);
	if (fd_mixer < 0) {
		printf("Error opening /dev/sound/mixer");
		return 1;
	}
	delay = 1;
	vol = SOUND_MASK_VOLUME;
	res = ioctl(fd_mixer, SOUND_MIXER_WRITE_OUTSRC, &vol);
	if (res == -1) {
		printf("SOUND_MIXER_WRITE_OUTSRC ioctl error: %d\n", errno);
		return -1;
	}
	vol = 0;
	res = ioctl(fd_mixer, SOUND_MIXER_READ_VOLUME, &vol);
	if (res == -1) {
		printf("SOUND_MIXER_READ_VOLUME ioctl error: %d\n", errno);
		return -1;
	}
	left = (vol & 0x00ff);
	right = ((vol & 0xff00) >> 8);
	printf("Current output device volume = %d, %d\n", left, right);
	while (tmp <= 240) {
		printf("Applying volume = %d, %d\n", left, right);
		vol = ((left << 8) | right);
		res = ioctl(fd_mixer, SOUND_MIXER_WRITE_VOLUME, &vol);
		if (res == -1) {
			printf("SOUND_MIXER_WRITE_VOLUME ioctl error: %d\n",
				errno);
			return -1;
		}
		if (right == 100) {
			down = 1;
		}
		if (right == 0) {
			down = 0;
		}
		if (down == 1) {
			left -= 5;
			right -= 5;
		} else {
			left += 5;
			right += 5;
		}
		sleep(delay);
		tmp += delay;
	}
	return 0;
}


