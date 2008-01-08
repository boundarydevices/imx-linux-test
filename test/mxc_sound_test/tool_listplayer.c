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

	/* Flush output */
	write(fd_audio, NULL, 0);

	return err;
}

/* Must put '\0' in "file" if error */
void get_next_file_name(FILE * fd_list, char *file_name)
{
	int ret;
	*file_name = '\0';

	/* get next line */
	ret = fscanf(fd_list, "%s\n", file_name);
	printf("Next file = %s\n", file_name);

//        if ( (ret == 0) || (ret == EOF) ) return;
}

int main(int ac, char *av[])
{
	FILE *fd_file, *fd_list;
	int fd_audio, fd_audio2, open_flag;
	int i, j, loop, ssi_index, liip;
	int err = 0;
	char file_name[256];

	printf("Hi... \n");

	if (ac == 1) {
		printf
		    ("main <ssi_nb> <playlist.txt> <file_loops> <list_loop> [device]\n Play files to /dev/dspX (uncompressed data)\n");
		printf("ssi_nb : 1 for SSI1 ... \n");
		printf("device : 0 for StDac, 1 for Codec... \n");
		return 0;
	}

	loop = atoi(av[3]);
	liip = atoi(av[4]);

	if (ac == 6) {
		if (atoi(av[5]) == 0)
			open_flag = O_WRONLY;
		else
			open_flag = O_RDWR;
	} else {
		open_flag = O_WRONLY;
	}

	if ((fd_list = fopen(av[2], "r")) <= 0) {
		err++;
		goto _err_list_not_found;
	}

	ssi_index = atoi(av[1]);
	if ((fd_audio = open("/dev/sound/dsp", O_WRONLY)) < 0) {
		printf("Error opening dsp");
		err++;
		goto _err_drv_open;
	}

	if (ssi_index == 2) {
		fd_audio2 = fd_audio;
		if ((fd_audio = open("/dev/sound/dsp1", O_WRONLY)) < 0) {
			printf("Error opening dsp1");
			err++;
			goto _err_drv_open2;
		}
	}

	/*! Loop on playlist */
	for (j = 0; j < liip; j++) {
		get_next_file_name(fd_list, file_name);

		while (*file_name != '\0') {

			if (*file_name == '\0')
				continue;

			if ((fd_file = fopen(file_name, "r")) <= 0) {
				printf("Error at file opening. Skip.\n");
				err++;
				goto _next;
			}

			if (set_audio_config(fd_audio, fd_file) < 0) {
				printf("Bad format for file %s. Skip.\n",
				       file_name);
				fclose(fd_file);
				err++;
				goto _next;
			}

			/* Loop on single file */
			for (i = 0; i < loop; i++) {
				err += play_file(fd_audio, fd_file);
			}	/* end play on single file */

			fclose(fd_file);
		      _next:
			get_next_file_name(fd_list, file_name);
		}

		fseek(fd_list, 0, SEEK_SET);

	}			/* end loop on playlist */

	printf("************** Closing the device\n");
	fclose(fd_list);
	close(fd_audio);
	if (ssi_index == 2)
		close(fd_audio2);

	if (err)
		goto _end_err;
	printf("All tests passed with success\n");
	return 0;

      _err_drv_open2:
	close(fd_audio);
      _err_drv_open:
	fclose(fd_list);
      _end_err:
      _err_list_not_found:
	printf("Encountered %d errors\n", err);
	return err;
}
