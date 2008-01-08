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

#define MAX_INSTANCES                   2

#define AUDIODEV_CODEC                  "/dev/sound/dsp"
#define AUDIODEV_STDAC                  "/dev/sound/dsp1"

int main(int ac, char *av[])
{
	int fd_audio[MAX_INSTANCES], i, err;
	err = 0;

	printf("Hi... \n");

	printf
	    ("This program checks that it is possible to open the driver for 2W and 1R \n");
	printf("-> main <no arg>\n\n");

	for (i = 0; i < MAX_INSTANCES; i++) {
		fd_audio[i] = -1;
	}

	/** 2 open in RDWR should fail */
	if ((fd_audio[0] = open(AUDIODEV_CODEC, O_RDWR)) < 0) {
		printf("[2 open in RDWR] Error at first opening\n");
		err++;
	}

	if ((fd_audio[1] = open(AUDIODEV_STDAC, O_RDWR)) >= 0) {
		printf("[2 open in RDWR] Error : 2RW allowed\n");
		err++;
	}

	for (i = 0; i < MAX_INSTANCES; i++) {
		if (fd_audio[i] >= 0) {
			close(fd_audio[i]);
			fd_audio[i] = -1;
		}
	}

	/** 2 open in WR should not fail */
	if ((fd_audio[0] = open(AUDIODEV_CODEC, O_WRONLY)) < 0) {
		printf("[2 open in WR] Error at first opening\n");
		err++;
	}

	if ((fd_audio[1] = open(AUDIODEV_STDAC, O_WRONLY)) < 0) {
		printf("[2 open in WR] Error at second opening\n");
		err++;
	}

	for (i = 0; i < MAX_INSTANCES; i++) {
		if (fd_audio[i] >= 0) {
			close(fd_audio[i]);
			fd_audio[i] = -1;
		}
	}

	/** second open in RD should fail */
	if ((fd_audio[0] = open(AUDIODEV_CODEC, O_RDONLY)) < 0) {
		printf("[2 open in RD] Error at first opening\n");
		err++;
	}

	if ((fd_audio[1] = open(AUDIODEV_STDAC, O_RDONLY)) >= 0) {
		printf("[2 open in RD] Error : 2RW allowed\n");
		err++;
	}

	for (i = 0; i < MAX_INSTANCES; i++) {
		if (fd_audio[i] >= 0) {
			close(fd_audio[i]);
			fd_audio[i] = -1;
		}
	}

	/** 1 RD + 1 WR should not fail */
	if ((fd_audio[0] = open(AUDIODEV_CODEC, O_RDONLY)) < 0) {
		printf("[1 RD + 1 WR] Error at first opening\n");
		err++;
	}

	if ((fd_audio[1] = open(AUDIODEV_STDAC, O_WRONLY)) < 0) {
		printf("[1 RD + 1 WR] Error at second opening\n");
		err++;
	}

	for (i = 0; i < MAX_INSTANCES; i++) {
		if (fd_audio[i] >= 0) {
			close(fd_audio[i]);
			fd_audio[i] = -1;
		}
	}

	/** 1 RDWR + 1 WR should not fail */
	if ((fd_audio[0] = open(AUDIODEV_CODEC, O_RDWR)) < 0) {
		printf("[1 RDWR + 1 WR] Error at first opening\n");
		err++;
	}

	if ((fd_audio[1] = open(AUDIODEV_STDAC, O_WRONLY)) < 0) {
		printf("[1 RDWR + 1 WR] Error at second opening\n");
		err++;
	}

	for (i = 0; i < MAX_INSTANCES; i++) {
		if (fd_audio[i] >= 0) {
			close(fd_audio[i]);
			fd_audio[i] = -1;
		}
	}

	/** 1 RD + 1 RDWR should fail */
	if ((fd_audio[0] = open(AUDIODEV_CODEC, O_RDONLY)) < 0) {
		printf("[1 RD + 1 RDWR] Error at first opening\n");
		err++;
	}

	if ((fd_audio[1] = open(AUDIODEV_STDAC, O_RDWR)) >= 0) {
		printf("[1 RD + 1 RDWR] Error : second opening should fail\n");
		err++;
	}

	for (i = 0; i < MAX_INSTANCES; i++) {
		if (fd_audio[i]) {
			close(fd_audio[i]);
			fd_audio[i] = -1;
		}
	}

	/** 1 RDWR + 1 RD should fail */
	if ((fd_audio[0] = open(AUDIODEV_CODEC, O_RDWR)) < 0) {
		printf("[1 RDWR + 1 RD] Error at first opening\n");
		err++;
	}

	if ((fd_audio[1] = open(AUDIODEV_STDAC, O_RDONLY)) >= 0) {
		printf("[1 RDWR + 1 RD] Error : second opening should fail\n");
		err++;
	}

	for (i = 0; i < MAX_INSTANCES; i++) {
		if (fd_audio[i] >= 0) {
			close(fd_audio[i]);
			fd_audio[i] = -1;
		}
	}

	if (err == 0) {
		printf("\n --> All the tests were passed successfully\n");
	} else {
		printf("\nEncountered %i errors\n", err);
	}

	return 0;

}
