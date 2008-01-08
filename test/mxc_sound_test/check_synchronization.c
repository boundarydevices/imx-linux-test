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

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include <unistd.h>
#include <stdio.h>

#define device   "/dev/sound/dsp1"
#define ws       8000
#define bits     16
#define channels 1
#define length   20

static signed short buffer[ws * length / 1000];

int main(void)
{
	int fd;
	int setting;
	int status;

	fd = open(device, O_RDONLY);
	if (fd < 0) {
		printf("snd: open FAILED!\n");
		return -1;
	}

	status = ioctl(fd, SNDCTL_DSP_GETCAPS, &setting);
	if (status != 0) {
		printf("snd: ioctl SNDCTL_DSP_GETCAPS FAILED!\n");
	}

	status = (setting & DSP_CAP_TRIGGER) != DSP_CAP_TRIGGER;
	if (status != 0) {
		printf
		    ("snd: querying DSP_CAP_TRIGGER FAILED (not supported?)!\n");
	}

	setting = PCM_ENABLE_INPUT;
	status = ioctl(fd, SNDCTL_DSP_SETTRIGGER, &setting);
	if (status != 0) {
		printf("snd: ioctl SNDCTL_DSP_SETTRIGGER FAILED!\n");
	}

	setting = bits;
	status = ioctl(fd, SNDCTL_DSP_SETFMT, &setting);
	if (status != 0) {
		printf("snd: ioctl SNDCTL_DSP_SETFMT FAILED!\n");
	}

	setting = ws;
	status = ioctl(fd, SOUND_PCM_WRITE_RATE, &setting);
	if (status) {
		printf("snd: ioctl SOUND_PCM_WRITE_RATE FAILED!\n");
	}

	setting = channels;
	status = ioctl(fd, SNDCTL_DSP_CHANNELS, &setting);
	if (status != 0) {
		printf("snd: ioctl SNDCTL_DSP_CHANNELS FAILED!\n");
	}

	setting = PCM_ENABLE_INPUT;
	status = ioctl(fd, SNDCTL_DSP_SETTRIGGER, &setting);
	if (status != 0) {
		printf("snd: ioctl SNDCTL_DSP_SETTRIGGER FAILED!\n");
	}

	printf("snd: Looping... Break with Ctrl+C!\n");
	while (1) {
		status = read(fd, buffer, sizeof(buffer));
		if (status != sizeof(buffer)) {
			printf("snd: read FAILED, ret=%d, errno=%d!\n", status,
			       errno);
		}

		/*
		   status = ioctl( fd, SNDCTL_DSP_GETIPTR, &info );
		   if( status != 0 )
		   {
		   printf( "snd: SNDCTL_DSP_GETIPTR FAILED, return=%d, errorno=%d!\n", status, errno );
		   }
		   else
		   {
		   printf( "snd: info.bytes=%d\n", info.bytes );
		   }
		 */
	}

	close(fd);
	return 0;
}
