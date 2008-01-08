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
#include "mxc-ctrls.h"		/* header for usefull oss and mc13783 ioctl plus mask parameters */

#define BUF_SIZE 32767

void write_wav_header(FILE * fd_file, int *totalread, unsigned short *chan,
		      int *frequency, unsigned short *bits)
{
	char riff[] = { 'R', 'I', 'F', 'F' };
	char wave_fmt[] = { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' };
	char data[] = { 'd', 'a', 't', 'a' };
	int file_size = *totalread + 44;
	int byte_rate = *frequency * (*bits / 8);
	int temp;

	fseek(fd_file, 0, SEEK_SET);
	fwrite(riff, 4, 1, fd_file);	/* pos 0 */
	fwrite(&file_size, 4, 1, fd_file);	/* pos 4 */
	fwrite(wave_fmt, 8, 1, fd_file);	/* pos 8 */
	temp = 0x00000010;
	fwrite(&temp, 4, 1, fd_file);	/* pos 16 */
	temp = 0x0001;
	fwrite(&temp, 2, 1, fd_file);	/* pos 20 */
	fwrite(chan, 2, 1, fd_file);	/* pos 22 */
	fwrite(frequency, 4, 1, fd_file);	/* pos 24 */
	fwrite(&byte_rate, 4, 1, fd_file);	/* pos 28 */
	temp = 0x0004;
	fwrite(&temp, 2, 1, fd_file);	/* pos 32 */
	fwrite(bits, 2, 1, fd_file);	/* pos 34 */
	fwrite(data, 4, 1, fd_file);	/* pos 36 */
	fwrite(totalread, 4, 1, fd_file);	/* pos 40 */
}

int main(int ac, char *av[])
{
	int fd_audio, fd_audio2, n, totalread, toread;
	FILE *fd_file;
	unsigned short buf[BUF_SIZE];
	int tmp, format, frequency, channels;
	unsigned short chan, bits;
	int val, err = 0, ssi_index;

	printf("Hi... \n");

	if (ac == 1) {
		printf
		    ("rec <ssi_nb> <soundfile.wav> <source> <bytes> [b f c] \n Records the file from /dev/dsp (uncompressed data)\n");
		printf(" ssi_nb : 1 for SSI1 or 2 for SSI2\n");
		printf(" source: 1:handset, 2:headset, 3: line in\n");
		printf("[b = bits number]\n");
		printf("[f = frequency]\n");
		printf("[c = channel count] \n");
		return 0;
	}

	toread = atoi(av[4]);

	ssi_index = atoi(av[1]);

	if ((fd_audio = open("/dev/sound/dsp", O_RDONLY)) < 0) {
		printf("Error opening /dev/sound/dsp");
		err++;
		goto _end_err;
	}

	if (ssi_index == 2) {
		if ((fd_audio2 = open("/dev/sound/dsp1", O_WRONLY)) < 0) {
			printf("Error opening /dev/sound/dsp1");
			close(fd_audio);
			err++;
			goto _end_err;
		}

		close(fd_audio);
		if ((fd_audio = open("/dev/sound/dsp", O_WRONLY)) < 0) {
			printf("Error opening /dev/sound/dsp");
			err++;
			close(fd_audio2);
			goto _end_err;
		}

		close(fd_audio2);
		fd_audio2 = fd_audio;
		if ((fd_audio = open("/dev/sound/dsp1", O_RDONLY)) < 0) {
			printf("Error opening /dev/sound/dsp1");
			err++;
			close(fd_audio2);
			goto _end_err;
		}
	}

	if ((fd_file = fopen(av[2], "w+")) <= 0) {
		err++;
		goto _err_file_not_found;
	}

	if (ac == 8) {
		// On configure le device audio
		tmp = format = atoi(av[5]);
		if (ioctl(fd_audio, SNDCTL_DSP_SETFMT, &format) < 0)
			err++;
		if (tmp != format)
			err++;

		tmp = frequency = atoi(av[6]);
		if (ioctl(fd_audio, SOUND_PCM_WRITE_RATE, &frequency) < 0)
			err++;
		if (tmp != frequency)
			err++;

		tmp = channels = atoi(av[7]);
		if (ioctl(fd_audio, SOUND_PCM_WRITE_CHANNELS, &channels) < 0)
			err++;
		if (tmp != channels)
			err++;

		printf("Will rec the file with the configuration %i %i %i \n",
		       format, frequency, channels);
		if (err)
			goto _err_fmt;
	}

	if ((ac == 5) || (ac == 4)) {
		printf("Will rec the file with the default HW configuration\n");

		// Query the HW its current format
		format = 0;
		if (ioctl(fd_audio, SNDCTL_DSP_SETFMT, &format) < 0)
			err++;

		frequency = 0;
		if (ioctl(fd_audio, SOUND_PCM_WRITE_RATE, &frequency) < 0)
			err++;

		channels = 0;
		if (ioctl(fd_audio, SNDCTL_DSP_CHANNELS, &channels) < 0)
			err++;

		printf("Will rec the file with the configuration %i %i %i \n",
		       format, frequency, channels);
		if (err)
			goto _err_fmt;
	}

	/*! set the output src */
	switch (atoi(av[3])) {
	case 1:
		val = SOUND_MASK_PHONEIN;
		if (ioctl(fd_audio, SOUND_MIXER_WRITE_RECSRC, &val) < 0)
			err++;
		break;
	case 2:
		val = SOUND_MASK_MIC;
		if (ioctl(fd_audio, SOUND_MIXER_WRITE_RECSRC, &val) < 0)
			err++;
		break;
	case 3:
		val = SOUND_MASK_LINE;
		if (ioctl(fd_audio, SOUND_MIXER_WRITE_RECSRC, &val) < 0)
			err++;
		break;
	default:
		printf("Unknown out src (%d), using the default one\n",
		       atoi(av[3]));
	}

	if (ac == 8) {
		tmp = channels = atoi(av[7]);
		if (ioctl(fd_audio, SOUND_PCM_WRITE_CHANNELS, &channels) < 0)
			err++;
		if (tmp != channels)
			err++;
		if (err)
			goto _err_fmt;
	}

	if ((ac == 5) || (ac == 4)) {
		channels = 0;
		if (ioctl(fd_audio, SNDCTL_DSP_CHANNELS, &channels) < 0)
			err++;
		if (err)
			goto _err_fmt;
	}
	// On passe l'entete WAV, qu'on remplira plus tard
	totalread = 0;
	memset(buf, 0, sizeof(buf));
	n = fwrite(buf, 44, 1, fd_file);

	while (toread > 0) {
		// Lecture de l'échantillon
		if (toread > sizeof(buf)) {
			if ((n = read(fd_audio, buf, sizeof(buf))) <= 0) {
				printf("Error reading audio data \n");
				break;
			}
		} else {
			if ((n = read(fd_audio, buf, toread)) <= 0) {
				printf("Error reading audio data \n");
				break;
			}
		}

		printf("Read %d bytes from the device\n", n);
		totalread += n;
		toread -= n;

		// On envoit l'échantillon...
		if ((n = fwrite(buf, 1, n, fd_file)) <= 0)
			perror("/dev/audio\n");
	}

	// On fini d'ecrire le fichier
	chan = channels;
	bits = format;
	printf(" totalread(%d), chan(%d), frequency(%d), bits(%d)", totalread,
	       chan, frequency, bits);
	write_wav_header(fd_file, &totalread, &chan, &frequency, &bits);

	printf("Data read = %08x\n", totalread);

	printf("************** Closing the device\n");
	fclose(fd_file);
	close(fd_audio);
	if (ssi_index == 2)
		close(fd_audio2);

	if (err)
		goto _end_err;
	printf("All tests passed with success\n");
	return 0;

      _err_fmt:
	close(fd_audio);
	fclose(fd_file);
	if (ssi_index == 2)
		close(fd_audio2);
      _end_err:
	printf("Encountered %d errors\n", err);
	return err;

      _err_file_not_found:
	printf("Error opening the file %s \n", av[2]);
	if (ssi_index == 2)
		close(fd_audio2);
	close(fd_audio);
	return 0;
}
