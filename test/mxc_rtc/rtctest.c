/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 *	Real Time Clock Driver Test/Example Program
 *
 *	Compile with:
 *		gcc -s -Wall -Wstrict-prototypes rtctest.c -o rtctest
 *
 *	Copyright (C) 1996, Paul Gortmaker.
 *      Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 *	Released under the GNU General Public License, version 2,
 *	included herein by reference.
 *
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
#include <stdlib.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void usage(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   rtctest --full         = do all tests\n");
	fprintf(stderr, "   rtctest --no-periodic  = don't do periodic interrupt tests\n");
}

int main(int argc, char **argv)
{
	int i, fd, retval, irqcount = 0;
	int periodic_test = 1;
	unsigned long tmp, data;
	struct rtc_time rtc_tm;

	if (argc != 2) {
		usage();
		return 1;
	}

	if (strcmp(argv[1], "--no-periodic") == 0) {
		periodic_test = 0;
	} else if (strcmp(argv[1], "--full") != 0) {
		usage();
		return 1;
	}

	fd = open("/dev/rtc0", O_RDONLY);

	if (fd == -1) {
		perror("/dev/rtc0");
		exit(errno);
	}

	fprintf(stderr, "\n\t\t\tRTC Driver Test Example.\n\n");

	if (periodic_test != 0) {
		/* Turn on update interrupts (one per second) */
		retval = ioctl(fd, RTC_UIE_ON, 0);
		if (retval == -1) {
			perror("ioctl");
			exit(errno);
		}

		fprintf(stderr,
			"Counting 5 update (1/sec) interrupts from reading /dev/rtc0:");
		fflush(stderr);
		for (i = 1; i < 6; i++) {
			/* This read will block */
			retval = read(fd, &data, sizeof(unsigned long));
			if (retval == -1) {
				perror("read");
				exit(errno);
			}
			fprintf(stderr, " %d", i);
			fflush(stderr);
			irqcount++;
		}

		fprintf(stderr, "\nAgain, from using select(2) on /dev/rtc0:");
		fflush(stderr);
		for (i = 1; i < 6; i++) {
			struct timeval tv = { 5, 0 };	/* 5 second timeout on select */
			fd_set readfds;

			FD_ZERO(&readfds);
			FD_SET(fd, &readfds);
			/* The select will wait until an RTC interrupt happens. */
			retval = select(fd + 1, &readfds, NULL, NULL, &tv);
			if (retval == -1) {
				perror("select");
				exit(errno);
			}
			/* This read won't block unlike the select-less case above. */
			retval = read(fd, &data, sizeof(unsigned long));
			if (retval == -1) {
				perror("read");
				exit(errno);
			}
			fprintf(stderr, " %d", i);
			fflush(stderr);
			irqcount++;
		}

		/* Turn off update interrupts */
		retval = ioctl(fd, RTC_UIE_OFF, 0);
		if (retval == -1) {
			perror("ioctl");
			exit(errno);
		}
	}

	/* Read the RTC time/date */
	retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
	if (retval == -1) {
		perror("ioctl");
		exit(errno);
	}

	fprintf(stderr,
		"\n\nCurrent RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",
		rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
		rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

	/* Set the alarm to 5 sec in the future, and check for rollover */
	rtc_tm.tm_sec += 5;
	if (rtc_tm.tm_sec >= 60) {
		rtc_tm.tm_sec %= 60;
		rtc_tm.tm_min++;
	}
	if (rtc_tm.tm_min == 60) {
		rtc_tm.tm_min = 0;
		rtc_tm.tm_hour++;
	}
	if (rtc_tm.tm_hour == 24)
		rtc_tm.tm_hour = 0;

	retval = ioctl(fd, RTC_ALM_SET, &rtc_tm);
	if (retval == -1) {
		perror("ioctl");
		exit(errno);
	}

	/* Read the current alarm settings */
	retval = ioctl(fd, RTC_ALM_READ, &rtc_tm);
	if (retval == -1) {
		perror("ioctl");
		exit(errno);
	}

	fprintf(stderr, "Alarm time now set to %02d:%02d:%02d.\n",
		rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

	/* Enable alarm interrupts */
	retval = ioctl(fd, RTC_AIE_ON, 0);
	if (retval == -1) {
		perror("ioctl");
		exit(errno);
	}

	fprintf(stderr, "Waiting 5 seconds for alarm...");
	fflush(stderr);
	/* This blocks until the alarm ring causes an interrupt */
	retval = read(fd, &data, sizeof(unsigned long));
	if (retval == -1) {
		perror("read");
		exit(errno);
	}
	irqcount++;
	fprintf(stderr, " okay. Alarm rang.\n");

	/* Disable alarm interrupts */
	retval = ioctl(fd, RTC_AIE_OFF, 0);
	if (retval == -1) {
		perror("ioctl");
		exit(errno);
	}

	if (periodic_test != 0) {
		/* Read periodic IRQ rate */
		retval = ioctl(fd, RTC_IRQP_READ, &tmp);
		if (retval == -1) {
			perror("ioctl");
			exit(errno);
		}
		fprintf(stderr, "\nPeriodic IRQ rate was %ldHz.\n", tmp);

		fprintf(stderr, "Counting 20 interrupts at:");
		fflush(stderr);

		/* The frequencies 128Hz, 256Hz, ... 8192Hz are only allowed for root. */
		for (tmp = 2; tmp <= 64; tmp *= 2) {

			retval = ioctl(fd, RTC_IRQP_SET, tmp);
			if (retval == -1) {
				perror("ioctl");
				exit(errno);
			}

			fprintf(stderr, "\n%ldHz:\t", tmp);
			fflush(stderr);

			/* Enable periodic interrupts */
			retval = ioctl(fd, RTC_PIE_ON, 0);
			if (retval == -1) {
				perror("ioctl");
				exit(errno);
			}

			for (i = 1; i < 21; i++) {
				/* This blocks */
				retval = read(fd, &data, sizeof(unsigned long));
				if (retval == -1) {
					perror("read");
					exit(errno);
				}
				fprintf(stderr, " %d", i);
				fflush(stderr);
				irqcount++;
			}

			/* Disable periodic interrupts */
			retval = ioctl(fd, RTC_PIE_OFF, 0);
			if (retval == -1) {
				perror("ioctl");
				exit(errno);
			}
		}
	}

	fprintf(stderr, "\n\n\t\t\t *** Test complete ***\n");
	fprintf(stderr,
		"\nTyping \"cat /proc/interrupts\" will show %d more events on IRQ rtc.\n\n",
		irqcount);

	close(fd);
	return 0;
}				/* end main */
