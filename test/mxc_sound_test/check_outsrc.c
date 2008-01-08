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
//#include <linux/soundcard.h>
#include "soundcard.h"

#define BUF_SIZE 4096

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
	ioctl(fd_audio, SNDCTL_DSP_CHANNELS, &channels);
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

	// On passe l'entete WAV
	fseek(file, 44, SEEK_SET);

	// Lecture de l'échantillon
	while ((n = fread(buf, 1, sizeof(buf), file)) > 0) {
		nleft = n;
		p = buf;

		// On envoie l'échantillon...
		while (nleft) {
			if ((nwritten = write(fd_audio, p, nleft)) < 0) {
				perror("/dev/sound/dsp0 write error");
				err++;
			}
//                      else
//                              printf ("%d/%d written.\n", nwritten, nleft);

			nleft -= nwritten;
			p += nwritten;
		}
	}

	return err;
}

int main(int ac, char *av[])
{
	FILE *fd_file;
	int fd_audio, fd_audio2;
	int i, loop, ssi_index, out_src, val;
	int err = 0;
	char rep[32];
	char sounddevice[32];

	printf("Hi... \n");

	if (ac == 1) {
		printf
		    ("main <ssi_nb> <soundfile.wav> <outsrc> [nb_loops]\n Plays the file to /dev/dspX (uncompressed data)\n");
		printf("ssi_nb : 1 for SSI1 ... \n");
		printf
		    ("outsrc : 0: earpiece, 1:handsfree, 2:headset, 3: line out \n");
		return 0;
	}

	out_src = atoi(av[3]);

	if (ac == 5)
		loop = atoi(av[4]);
	else
		loop = 1;

	if ((fd_file = fopen(av[2], "r")) <= 0) {
		err++;
		goto _err_file_not_found;
	}

	if ((fd_audio = open("/dev/sound/dsp", O_WRONLY)) < 0) {
		printf("Error opening /dev/sound/dsp");
		err++;
		goto _err_drv_open;
	}

	ssi_index = atoi(av[1]);
	if (ssi_index == 2) {
		fd_audio2 = fd_audio;
		if ((fd_audio = open("/dev/sound/dsp1", O_WRONLY)) < 0) {
			printf("Error opening /dev/sound/dsp1");
			err++;
			goto _err_drv_open2;
		}
	}

	if (set_audio_config(fd_audio, fd_file) < 0) {
		printf("Bad format for file 1. ");
		printf("Do you want to continue ? (y/n) : ");
		scanf("%s", rep);
		err++;
		if (rep[0] != 'y')
			goto _err_fmt;
	}

	/*! set the output src */
	switch (out_src) {
	case 0:
		val = SOUND_MASK_PHONEOUT;
		if (ioctl(fd_audio, SOUND_MIXER_WRITE_OUTSRC, &val) < 0)
			err++;
		break;
	case 1:
		val = SOUND_MASK_SPEAKER;
		if (ioctl(fd_audio, SOUND_MIXER_WRITE_OUTSRC, &val) < 0)
			err++;
		break;
	case 2:
		val = SOUND_MASK_VOLUME;
		if (ioctl(fd_audio, SOUND_MIXER_WRITE_OUTSRC, &val) < 0)
			err++;
		break;
	case 3:
		val = SOUND_MASK_PCM;
		if (ioctl(fd_audio, SOUND_MIXER_WRITE_OUTSRC, &val) < 0)
			err++;
		break;
	default:
		printf("Unknown out src, using the default one\n");
	}

	// read the file many times if desired
	for (i = 0; i < loop; i++) {
		err += play_file(fd_audio, fd_file);
	}

	printf("************** Closing the device\n");
	fclose(fd_file);
	if (ssi_index == 2)
		close(fd_audio2);
	close(fd_audio);

	if (err)
		goto _end_err;
	printf("All tests passed with success\n");
	return 0;

      _err_fmt:
	if (ssi_index == 2)
		close(fd_audio2);
      _err_drv_open2:
	close(fd_audio);
      _err_drv_open:
	fclose(fd_file);
      _end_err:
	printf("Encountered %d errors\n", err);
	return err;

      _err_file_not_found:
	printf("Error opening the file %s \n", av[2]);
	return 0;
}
