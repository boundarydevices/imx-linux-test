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
	int fd_audio;
	int i, loop, bal;
	int err = 0;
	char rep[32];

	printf("Hi... \n");

	if ((ac != 3) && (ac != 4)) {
		printf
		    ("This program reconfigures the balance and plays a file\n");
		printf("main <soundfile.wav>  <bal>  [nb_loops] \n");
		return 0;
	}

	if (ac == 3) {
		loop = 1;
		bal = atoi(av[2]);
	} else {
		loop = atoi(av[3]);
		bal = atoi(av[2]);
	}

	if ((fd_audio = open("/dev/sound/dsp", O_WRONLY)) < 0) {
		printf("Error opening /dev/sound/dsp");
		err++;
		goto _err_drv_open;
	}

	/*! configuration of the balance, to be done AFTER the opening */
	ioctl(fd_audio, SNDCTL_MC13783_WRITE_OUT_BALANCE, &bal);

/*	printf("Want to configure the HW (y/n) ?\n"); scanf("%s", rep);
	while ( rep[0] != 'n' )
	{
		printf("R/W reg val (n 0 0 to exit)\n");
		scanf("%s %x %x", rep, &reg, &val);
		cfg.reg = reg; cfg.val = val;
		if ( rep[0] == 'R' ) printf("Read returned = %x\n", ioctl(fd_audio, SNDCTL_DBMX_HW_SSI_CFG_R, &cfg));
		if ( rep[0] == 'W' ) printf("Write returned = %x\n", ioctl(fd_audio, SNDCTL_DBMX_HW_SSI_CFG_W, &cfg));
	}*/

	if ((fd_file = fopen(av[1], "r")) <= 0) {
		err++;
		goto _err_file_not_found;
	}

	if (set_audio_config(fd_audio, fd_file) < 0) {
		printf("Bad format for file 1. ");
		printf("Do you want to continue ? (y/n) : ");
		scanf("%s", rep);
		err++;
		if (rep[0] != 'y')
			goto _err_fmt;
	}
	// read the file many times if desired
	for (i = 0; i < loop; i++) {
		err += play_file(fd_audio, fd_file);
	}

	printf("************** Closing the device\n");
	fclose(fd_file);
	close(fd_audio);

	if (err)
		goto _end_err;
	printf("All tests passed with success\n");
	return 0;

      _err_fmt:
	close(fd_audio);
      _err_drv_open:
	fclose(fd_file);
      _end_err:
	printf("Encountered %d errors\n", err);
	return err;

      _err_file_not_found:
	printf("Error opening the file %s \n", av[1]);
	close(fd_audio);
	return 0;
}
