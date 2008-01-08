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
#include "dbmx31-ctrls.h"

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

#define ARG_SSI         1
#define ARG_FILE_S      2
#define ARG_FILE_C      3

int main(int ac, char *av[])
{
	FILE *fd_file;
	int fd_audio, fd_audio2;
	int err = 0, val;

	printf("Hi... \n");

	if (ac != 4) {
		printf("This program checks that it is possible to choose ");
		printf("the device on which to play (StDAC or CODEC)\n");
		printf
		    ("It plays on StDAC then CODEC and StDAC again (checks device toggling)\n");
		printf("-> main <SSI> <file4StDAC.wav> <file4CODEC.wav>\n\n");
		printf("   SSI: 1 for SSI1, 2 for SSI2\n");
		return 0;
	}

	if ((fd_audio = open("/dev/sound/dsp", O_WRONLY)) < 0) {
		printf("Error opening /dev/dsp");
		return 0;
	}

	if (atoi(av[ARG_SSI]) == 2) {
		fd_audio2 = fd_audio;
		if ((fd_audio = open("/dev/sound/dsp1", O_WRONLY)) < 0) {
			printf("Error opening /dev/dsp1");
			close(fd_audio2);
			return 0;
		}
	}

	/*
	 * Play on the StDac, to LINE-OUT
	 */
	val = SOUND_MASK_PCM;
	if (ioctl(fd_audio, SNDCTL_DBMX_HW_SEL_OUT_DAC, 0) < 0)
		err++;
	if (ioctl(fd_audio, SOUND_MIXER_WRITE_OUTSRC, &val) < 0)
		err++;

	if ((fd_file = fopen(av[ARG_FILE_S], "r")) <= 0) {
		goto _err_file_not_found;
	}

	if (set_audio_config(fd_audio, fd_file) < 0) {
		printf("Bad format for file 1\n");
		err++;
		goto _err_fmt;
	}

	err += play_file(fd_audio, fd_file);
	fclose(fd_file);

	/*
	 * Play on the CODEC, toggle from STDAC to CODEC
	 */
	val = SOUND_MASK_PCM;
	if (ioctl(fd_audio, SNDCTL_DBMX_HW_SEL_OUT_CODEC, 0) < 0)
		err++;
	if (ioctl(fd_audio, SOUND_MIXER_WRITE_OUTSRC, &val) < 0)
		err++;

	if ((fd_file = fopen(av[ARG_FILE_C], "r")) <= 0) {
		goto _err_file_not_found;
	}

	if (set_audio_config(fd_audio, fd_file) < 0) {
		printf("Bad format for file 1\n");
		err++;
		goto _err_fmt;
	}

	err += play_file(fd_audio, fd_file);
	fclose(fd_file);

	/*
	 * Play on the StDac, again, to toggle from CODEC to STDAC
	 */
	if (ioctl(fd_audio, SNDCTL_DBMX_HW_SEL_OUT_DAC, 0) < 0)
		err++;
	if (ioctl(fd_audio, SOUND_MIXER_WRITE_OUTSRC, &val) < 0)
		err++;

	if ((fd_file = fopen(av[ARG_FILE_S], "r")) <= 0) {
		goto _err_file_not_found;
	}

	if (set_audio_config(fd_audio, fd_file) < 0) {
		printf("Bad format for file 1\n");
		err++;
		goto _err_fmt;
	}

	err += play_file(fd_audio, fd_file);

	fclose(fd_file);
	close(fd_audio);

	if (err)
		goto _end_err;
	printf("All tests passed with success\n");
	return 0;

      _err_fmt:
	close(fd_audio);
      _end_err:
	printf("Encountered %d errors\n", err);
	return err;

      _err_file_not_found:
	printf("Error opening the file %s \n", av[ARG_FILE_S]);
	close(fd_audio);
	return 0;

}
