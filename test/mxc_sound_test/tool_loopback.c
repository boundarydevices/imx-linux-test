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
#include <pthread.h>
#include "dbmx31-ctrls.h"

#define BUF_SIZE        4096
 //32767
#define ARG_SRC         1
#define ARG_TYPE        2
#define ARG_BYTES       3
#define ARG_BITS        4
#define ARG_FREQ        5
#define ARG_CHAN        6

typedef struct _thrd_t {
	int drv_fd;
	int sample_nb;
} thrd_t;

char audio_buffer[4][BUF_SIZE];
int not_reached;
char *wr_ptr;
char *rd_ptr;

pthread_mutex_t thread_flag_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reached_flag_mutex = PTHREAD_MUTEX_INITIALIZER;

/* the recording thread */
void main1(thrd_t * local_arg)
{
	int nread;
	char *l_rd_ptr;
	int fd_audio = local_arg->drv_fd;
	int bytes = local_arg->sample_nb;

	bytes -= BUF_SIZE;

	printf("Enter main1\n");

	// On lit l'échantillon...
	while (bytes > 0) {
		l_rd_ptr = rd_ptr;

		if ((nread = read(fd_audio, l_rd_ptr, BUF_SIZE)) < 0)
			perror("/dev/sound/dsp write error");
		else
			printf("%d/%d read (%p).\n", nread, bytes, l_rd_ptr);

		bytes -= BUF_SIZE;

		/* increment reading pointer and maybe wrap */
		l_rd_ptr += BUF_SIZE;
		if (l_rd_ptr > &audio_buffer[3][0])
			l_rd_ptr = &audio_buffer[0][0];

		rd_ptr = l_rd_ptr;
	}

	/* Exit condition for play thread */
	pthread_mutex_lock(&reached_flag_mutex);
	not_reached = 0;
	pthread_mutex_unlock(&reached_flag_mutex);
}

/* the playing thread */
void main2(thrd_t * local_arg)
{
	int ntot, nwritten;
	char *l_wr_ptr;
	int fd_audio = local_arg->drv_fd;
	int cond;

	// On envoie l'échantillon...
	pthread_mutex_lock(&thread_flag_mutex);
	cond = not_reached;
	pthread_mutex_unlock(&thread_flag_mutex);

	printf("Enter main2\n");
	ntot = 0;

	while (cond) {
		pthread_mutex_lock(&thread_flag_mutex);
		l_wr_ptr = wr_ptr;
		pthread_mutex_unlock(&thread_flag_mutex);

		if ((nwritten = write(fd_audio, l_wr_ptr, BUF_SIZE)) < 0)
			perror("/dev/sound/dsp write error");
		else {
			ntot += BUF_SIZE;
			printf("%d/%d written (%p).\n", nwritten, ntot,
			       l_wr_ptr);
		}

		/* Recompute writing pointer according to new reading pointer */
		pthread_mutex_lock(&thread_flag_mutex);
		wr_ptr += BUF_SIZE;
		if (wr_ptr > &audio_buffer[3][0])
			wr_ptr = &audio_buffer[0][0];
		pthread_mutex_unlock(&thread_flag_mutex);

		pthread_mutex_lock(&reached_flag_mutex);
		cond = not_reached;
		pthread_mutex_unlock(&reached_flag_mutex);
	}
}

/*!
 * This function launches two threads. One for reading, one for writing
 * @param fd_audio the reading device
 * @param fd_audio2 the writing device
 * @param toread the number of bytes to read
 * @return the number of errors encountered
 */
int launch_full_duplex(int fd_audio, int fd_audio2, int toread)
{
	thrd_t local_arg, local_arg2;
	pthread_t thread1, thread2;
	int ret;

	/* Init variables */
	memset(audio_buffer, 0, 4 * BUF_SIZE);
	not_reached = 1;
	wr_ptr = &audio_buffer[0][0];
	rd_ptr = &audio_buffer[1][0];

	/* Initialize the mutex and condition variable.
	   pthread_mutex_init (&thread_flag_mutex, NULL); */

	printf("Create threads 1 and 2 (read %d bytes) \n", toread);

	local_arg.drv_fd = fd_audio;
	local_arg.sample_nb = toread;
	ret =
	    pthread_create(&thread1, NULL, (void *)&main1, (void *)&local_arg);

	local_arg2.drv_fd = fd_audio2;
	local_arg2.sample_nb = toread;
	ret =
	    pthread_create(&thread2, NULL, (void *)&main2, (void *)&local_arg2);
	ret = pthread_join(thread1, NULL);
	ret = pthread_join(thread2, NULL);

	return ret;
}

int main(int ac, char *av[])
{
	int tmp, format, frequency, channels;
	int val, err = 0;
	int fd_audio, fd_audio2, toread;

	printf("Hi... \n");

	if (ac == 1) {
		printf
		    ("loopback <source> <type> <bytes> [b f c] \n Plays data recorded (loopback at SSI level)\n");
		printf(" source: 1:handset, 2:headset, 3: line in\n");
		printf(" type: 1:kernel space loopback(SSI1)\n");
		printf(" type: 2:kernel space loopback(SSI2)\n");
		printf(" type: 3:user space loopback (SSI1)\n");
		printf(" type: 4:user space loopback (SSI2)\n");
		printf(" type: 5:user space loopback (SSI1->SSI2)\n");
		printf(" type: 6:user space loopback (SSI2->SSI1)\n");
		printf("[b = bits number]\n");
		printf("[f = frequency]\n");
		printf("[c = channel count] \n");
		return 0;
	}

	/* nb of bytes to read */
	toread = atoi(av[ARG_BYTES]);

	/* Open driver according to scenario
	   fd_audio will record, fd_audio will play */
	switch (atoi(av[ARG_TYPE])) {
	case (1):		/* use SSI1, in SSI registers directly */
		if ((fd_audio = open("/dev/sound/dsp", O_RDWR)) < 0) {
			printf("Error opening /dev/dsp");
			return 0;
		}
		fd_audio2 = fd_audio;
		break;
	case (2):		/* use SSI2, in SSI registers directly */
		if ((fd_audio2 = open("/dev/sound/dsp", O_RDONLY)) < 0) {
			printf("Error opening /dev/dsp");
			return 0;
		}
		if ((fd_audio = open("/dev/sound/dsp1", O_WRONLY)) < 0) {
			printf("Error opening /dev/dsp1");
			close(fd_audio2);
			return 0;
		}
		close(fd_audio2);
		if ((fd_audio2 = open("/dev/sound/dsp", O_WRONLY)) < 0) {
			printf("Error opening /dev/dsp");
			return 0;
		}
		close(fd_audio);
		if ((fd_audio = open("/dev/sound/dsp1", O_RDWR)) < 0) {
			printf("Error opening /dev/dsp1");
			close(fd_audio2);
			return 0;
		}
		break;
	case (3):		/* use SSI1, full duplex in user space */
		if ((fd_audio = open("/dev/sound/dsp", O_RDWR)) < 0) {
			printf("Error opening /dev/dsp");
			return 0;
		}
		fd_audio2 = fd_audio;
		break;
	case (4):		/* use SSI2, full duplex in user space */
		if ((fd_audio2 = open("/dev/sound/dsp", O_WRONLY)) < 0) {
			printf("Error opening /dev/dsp");
			return 0;
		}
		/* Use the codec instead of stdac */
		if (ioctl(fd_audio2, SNDCTL_DBMX_HW_SEL_OUT_CODEC, 0) < 0)
			err++;

		if ((fd_audio = open("/dev/sound/dsp1", O_RDWR)) < 0) {
			printf("Error opening /dev/dsp1");
			close(fd_audio2);
			return 0;
		}
		break;
	case (5):		/* use SSI1 and SSI2, full duplex in user space */
		if ((fd_audio = open("/dev/sound/dsp", O_RDONLY)) < 0) {
			printf("Error opening /dev/dsp");
			return 0;
		}

		if ((fd_audio2 = open("/dev/sound/dsp1", O_WRONLY)) < 0) {
			printf("Error opening /dev/dsp1");
			close(fd_audio);
			return 0;
		}
		break;
	case (6):		/* use SSI2 and SSI1, full duplex in user space */
		if ((fd_audio2 = open("/dev/sound/dsp", O_WRONLY)) < 0) {
			printf("Error opening /dev/dsp");
			return 0;
		}
		/* Use the codec instead of stdac */
		if (ioctl(fd_audio2, SNDCTL_DBMX_HW_SEL_OUT_CODEC, 0) < 0)
			err++;

		if ((fd_audio = open("/dev/sound/dsp1", O_RDONLY)) < 0) {
			printf("Error opening /dev/dsp1");
			close(fd_audio2);
			return 0;
		}
		break;
	default:
		printf("Not implemented yet\n");
		return 0;
	}

	/*! set the output src */
	switch (atoi(av[ARG_SRC])) {
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
		       atoi(av[ARG_SRC]));
	}

	if (ac == 7) {
		// On configure le device audio
		tmp = format = atoi(av[ARG_BITS]);
		if (ioctl(fd_audio, SNDCTL_DSP_SETFMT, &format) < 0)
			err++;
		if (tmp != format)
			err++;
		if (ioctl(fd_audio2, SNDCTL_DSP_SETFMT, &format) < 0)
			err++;
		if (tmp != format)
			err++;

		tmp = frequency = atoi(av[ARG_FREQ]);
		if (ioctl(fd_audio, SOUND_PCM_WRITE_RATE, &frequency) < 0)
			err++;
		if (tmp != frequency)
			err++;
		if (ioctl(fd_audio2, SOUND_PCM_WRITE_RATE, &frequency) < 0)
			err++;
		if (tmp != frequency)
			err++;

		tmp = channels = atoi(av[ARG_CHAN]);
		if (ioctl(fd_audio, SOUND_PCM_WRITE_CHANNELS, &channels) < 0)
			err++;
		if (tmp != channels)
			err++;
		if (ioctl(fd_audio2, SOUND_PCM_WRITE_CHANNELS, &channels) < 0)
			err++;
		if (tmp != channels)
			err++;

		printf("Will rec the file with the configuration %i %i %i \n",
		       format, frequency, channels);
		if (err)
			goto _err_fmt;
	} else {
		printf("Will rec the file with the default HW configuration\n");

		// Query the HW its current format
		format = 0;
		if (ioctl(fd_audio, SNDCTL_DSP_SETFMT, &format) < 0)
			err++;
		format = 0;
		if (ioctl(fd_audio2, SNDCTL_DSP_SETFMT, &format) < 0)
			err++;

		frequency = 0;
		if (ioctl(fd_audio, SOUND_PCM_WRITE_RATE, &frequency) < 0)
			err++;
		frequency = 0;
		if (ioctl(fd_audio2, SOUND_PCM_WRITE_RATE, &frequency) < 0)
			err++;

		channels = 0;
		if (ioctl(fd_audio, SNDCTL_DSP_CHANNELS, &channels) < 0)
			err++;
		channels = 0;
		if (ioctl(fd_audio2, SNDCTL_DSP_CHANNELS, &channels) < 0)
			err++;

		printf("Will rec the file with the configuration %i %i %i \n",
		       format, frequency, channels);
		if (err)
			goto _err_fmt;
	}

	if (atoi(av[ARG_TYPE]) <= 2) {
		/* Call the kernel driver loopback control */
		if (ioctl(fd_audio, SNDCTL_DBMX_HW_SSI_LOOPBACK, &toread) < 0)
			err++;
	} else {
		/* Make a loopback at user space level */
		if (launch_full_duplex(fd_audio, fd_audio2, toread))
			err++;
	}

	printf("************** Closing the device\n");
	/* Close driver according to scenario */
	switch (atoi(av[ARG_TYPE])) {
	case (1):
	case (3):
		close(fd_audio);
		break;
	default:
		close(fd_audio);
		close(fd_audio2);
	}

	if (err)
		goto _end_err;
	printf("All tests passed with success\n");
	return 0;

      _err_fmt:
	close(fd_audio);
	close(fd_audio2);
      _end_err:
	printf("Encountered %d errors\n", err);
	return err;
}
