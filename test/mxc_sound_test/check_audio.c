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

#include "soundcard.h"
#include "audio_controls.h"

#define BUF_SIZE 4096

void help_info(int ac, char *av[])
{
	printf("\n\n**************************************************\n");
	printf("* Test aplication for sound playback \n");
	printf("* Options : \n\n");
	printf
	    ("%s <audio-device> <soundfile.wav> \n Plays an audio file in /dev/dspX\n",
	     av[0]);
#ifdef CONFIG_MX21ADS
	printf("audio-device : 0 for CODEC (/dev/sound/dsp)\n");
#else
	printf
	    ("audio-device : 0 for STEREODAC (/dev/sound/dsp), 1 for CODEC (/dev/sound/dsp1)\n");
#endif
	printf("**************************************************\n\n");
}

unsigned short get_audio_bits(FILE * file)
{
	unsigned short ret;
	fseek(file, 34, SEEK_SET);
	fread(&ret, 2, 1, file);
	return ret;
}

unsigned short get_audio_channels(FILE * file)
{
	unsigned short ret;
	fseek(file, 22, SEEK_SET);
	fread(&ret, 2, 1, file);
	return ret;
}

int get_audio_frq(FILE * file)
{
	int ret;
	fseek(file, 24, SEEK_SET);
	fread(&ret, 4, 1, file);
	return ret;
}

int set_audio_config(int fd_audio, FILE * file)
{
	int tmp, format, frequency, channels;

	tmp = format = (int)get_audio_bits(file);
	ioctl(fd_audio, SNDCTL_DSP_SETFMT, &format);
	if (tmp != format)
		return -1;

	tmp = frequency = get_audio_frq(file);
	ioctl(fd_audio, SOUND_PCM_WRITE_RATE, &frequency);
	if (tmp != frequency)
		return -1;

	tmp = channels = get_audio_channels(file);
	ioctl(fd_audio, SOUND_PCM_WRITE_CHANNELS, &channels);
	if (tmp != channels)
		return -1;

	printf("Configuration : %d %d %d\n", format, frequency, channels);
	return 0;
}

int play_file(int fd_audio, FILE * file)
{
	int err = 0;
	int n, nleft, nwritten;
	char buf[BUF_SIZE], *p;

	/* On passe l'entete WAV */
	fseek(file, 44, SEEK_SET);

	/* Lecture de l'échantillon */
	while ((n = fread(buf, 1, sizeof(buf), file)) > 0) {
		nleft = n;
		p = buf;

		/* On envoie l'échantillon... */
		while (nleft) {
			if ((nwritten = write(fd_audio, p, nleft)) < 0) {
				perror("audio driver write error");
				err++;
			}

			nleft -= nwritten;
			p += nwritten;
		}
	}
	return err;
}

int main(int ac, char *av[])
{
	FILE *fd_file = NULL;
	int fd_audio;
	char rep[32];
	int err = 0;
	int ssi = 0;

	printf("Hi... \n");

	if (ac != 3) {
		help_info(ac, av);
		return 1;
	}
#ifdef CONFIG_MX21ADS
	ssi = 0;
#else
	ssi = atoi(av[1]);
#endif
	if ((ssi != 0) && (ssi != 1)) {
		printf("unrecognized parameter for audio channel\n");
		help_info(ac, av);
		return 1;
	}

	if (ssi == 0) {
		printf("playing on /dev/sound/adsp (SSI1)\n");
		fd_audio = open("/dev/sound/adsp", O_WRONLY);
	} else {
		printf("playing on /dev/sound/dsp (SSI2)\n");
		fd_audio = open("/dev/sound/dsp", O_WRONLY);
	}

	if (fd_audio < 0) {
		printf("Error opening device file\n");
		err++;
		goto end_err;
	}

	if ((fd_file = fopen(av[2], "r")) <= 0) {
		err++;
		goto err_file_not_found;
	}

	if (set_audio_config(fd_audio, fd_file) < 0) {
		printf("Bad format for file 1. ");
		printf("Do you want to continue ? (y/n) : ");
		scanf("%s", rep);
		err++;

		if (rep[0] != 'y') {
			goto err_fmt;
		}
	}


	/* Config HW */
	err += play_file(fd_audio, fd_file);

	printf("************** Closing the device\n");

	fclose(fd_file);
	close(fd_audio);

	if (err) {
		goto end_err;
	}

	printf("All tests passed with success\n");
	return 0;

      err_fmt:
	close(fd_audio);
//err_drv_open:
	fclose(fd_file);
      end_err:
	printf("Encountered %d errors\n", err);
	return err;

      err_file_not_found:
	printf("Error opening the file %s \n", av[1]);
	close(fd_audio);
	return 0;
}
