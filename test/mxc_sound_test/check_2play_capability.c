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
#include <pthread.h>

#define BUF_SIZE 4096

typedef struct _thrd_t {
	int drv_fd;
	FILE *file_fd;
	int max_loops;
} thrd_t;

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

void main1(thrd_t * local_arg)
{
	int n, nleft, nwritten, i;
	char buf[BUF_SIZE], *p;
	FILE *fd_file = local_arg->file_fd;
	int fd_audio = local_arg->drv_fd;
	int loop = local_arg->max_loops;

	if (set_audio_config(fd_audio, fd_file) < 0) {
		printf("Bad format for file 1\n");
		return;
	}
	// read the file many times if desired
	for (i = 0; i < loop; i++) {
		// On passe l'entete WAV
		fseek(fd_file, 44, SEEK_SET);

		// Lecture de l'échantillon
		while ((n = fread(buf, 1, sizeof(buf), fd_file)) > 0) {
			nleft = n;
			p = buf;

			// On envoie l'échantillon...
			while (nleft) {
				if ((nwritten = write(fd_audio, p, nleft)) < 0)
					perror("/dev/sound/dsp0 write error");
//                              else
//                                      printf ("%d/%d written.\n", nwritten, nleft);

				nleft -= nwritten;
				p += nwritten;
			}
		}
	}

	/* Flush output */
	write(fd_audio, NULL, 0);
}

void main2(thrd_t * local_arg)
{
	int n, nleft, nwritten, i;
	char buf[BUF_SIZE], *p;
	FILE *fd_file = local_arg->file_fd;
	int fd_audio = local_arg->drv_fd;
	int loop = local_arg->max_loops;

	if (set_audio_config(fd_audio, fd_file) < 0) {
		printf("Bad format for file 2\n");
		return;
	}
	// read the file many times if desired
	for (i = 0; i < loop; i++) {
		// On passe l'entete WAV
		fseek(fd_file, 44, SEEK_SET);

		// Lecture de l'échantillon
		while ((n = fread(buf, 1, sizeof(buf), fd_file)) > 0) {
			nleft = n;
			p = buf;

			// On envoie l'échantillon...
			while (nleft) {
				if ((nwritten = write(fd_audio, p, nleft)) < 0)
					perror("/dev/sound/dsp1 write error");
//                              else
//                                      printf ("%d/%d written.\n", nwritten, nleft);

				nleft -= nwritten;
				p += nwritten;
			}
		}
	}

	/* Flush output */
	write(fd_audio, NULL, 0);
}

#define ARG_DEV         1
#define ARG_FILE1       2
#define ARG_FILE2       3
#define ARG_LOOPS       4

int main(int ac, char *av[])
{
	FILE *fd_file, *fd_file2;
	int fd_audio, fd_audio2;
	int ret, open_flag;
	thrd_t local_arg, local_arg2;
	pthread_t thread1, thread2;

	printf("Hi... \n");

	if ((ac != 4) && (ac != 5)) {
		printf
		    ("This program checks that it is possible to play 2 files at a time\n");
		printf("-> main <device> <file1> <file2> [max_loops]\n\n");
		printf("        device: 0 for StDAC, 1 for codec\n");
		return 0;
	}

	if (atoi(av[ARG_DEV]) == 0)
		open_flag = O_WRONLY;
	else
		open_flag = O_RDWR;

	if ((fd_audio = open("/dev/sound/dsp", open_flag)) < 0) {
		printf("Error opening /dev/dsp");
		return 0;
	}

	if ((fd_audio2 = open("/dev/sound/dsp1", O_WRONLY)) < 0) {
		printf("Error opening /dev/dsp1");
		close(fd_audio);
		return 0;
	}

	if ((fd_file = fopen(av[ARG_FILE1], "r")) <= 0) {
		printf("Error opening the file %s \n", av[ARG_FILE1]);
		close(fd_audio2);
		close(fd_audio);
		return 0;
	}

	if ((fd_file2 = fopen(av[ARG_FILE2], "r")) <= 0) {
		printf("Error opening the file %s \n", av[ARG_FILE2]);
		fclose(fd_file);
		close(fd_audio2);
		close(fd_audio);
		return 0;
	}

	local_arg.drv_fd = fd_audio;
	local_arg.file_fd = fd_file;
	local_arg.max_loops = ((ac == 5) ? atoi(av[ARG_LOOPS]) : 1);
	ret =
	    pthread_create(&thread1, NULL, (void *)&main1, (void *)&local_arg);

	local_arg2.drv_fd = fd_audio2;
	local_arg2.file_fd = fd_file2;
	local_arg2.max_loops = ((ac == 5) ? atoi(av[ARG_LOOPS]) : 1);
	ret =
	    pthread_create(&thread2, NULL, (void *)&main2, (void *)&local_arg2);
	ret = pthread_join(thread1, NULL);
	ret = pthread_join(thread2, NULL);

	fclose(fd_file);
	fclose(fd_file2);
	close(fd_audio);
	close(fd_audio2);

	return 0;
}
