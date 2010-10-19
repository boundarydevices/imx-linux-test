/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 *	Real Time Clock Driver Test/Example Program
 *
 * rtc_wait_for_time_set_test -- test & demo the use of
 * RTC_WAIT_TIME_SET and RTC_READ_TIME_47BIT
 * ioctls in SRTC driver.
 *
 * The parent process, which simulates a DRM process, calls
 * RTC_WAIT_TIME_SET ioctl, which is a blocking call until
 * the time is set by the child process, which simulates a user.
 * The blocking ioctl returns upon time set with a difference
 * between new time counter value and old time counter value.
 * Processes such as DRM can utilize these ioctls to keep track of
 * changes to the SRTC in order to implement a secure clock:
 * secure clock value = SRTC counter value + offset, where offset
 * is kept in sync with any changes made to the SRTC clock counter.
 *
 * The child process, in this test, first sets time ahead by 16 seconds
 * and then sets the time back by 16 seconds. Each time, the parent process
 * is woken up, and it returns failure if the time difference is not +16 seconds
 * and -16 seconds, respectively. When time is changed by seconds, we check
 * for rollover in the minute and hour fields. However, rollover into the day field
 * is not checked. Therefore, if this test is run close to midnight, then it may fail.
 *
 * IMPORTANT: The DRM process, like the parent process in this test, must be
 * waiting to be notified before the time is changed, and it helps to have the
 * highest priority. Otherwise, the process that sets the time will not yield to the
 * DRM process immediately following the time set action, leading to a missed
 * report of the time change. In this test, the child process priority is set to a
 * much lower value than the parent's, whose "nice" value defaults to 0. Also,
 * the child yields the CPU in the beginning by sleeping to ensure that parent
 * can call the RTC_WAIT_TIME_SET ioctl before time is actually changed.
 * In a real system, the developer must ensure somehow that DRM process calls
 * the RTC_WAIT_TIME_SET ioctl to wait for notification before user has a
 * chance to change the time.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <linux/rtc.h>
#include <linux/mxc_srtc.h>

/* Nice value (inverse of priority): between -20 and 19 */
#define CHILD_PROC_NICE_VAL	15

int main(int argc, char **argv)
{
	int fd_srtc;
	unsigned long long time_47bit;
	long long time_diff;
	struct rtc_time rtc_tm;
	int retval = 0;

	fd_srtc = open("/dev/rtc0", O_RDONLY);

	if (fd_srtc == -1) {
		perror("/dev/rtc0");
		exit(errno);
	}

	fprintf(stderr, "\n\t\t\tRTC_WAIT_TIME_SET Test.\n\n");


	/* first, let's just read the time from SRTC */
	retval = ioctl(fd_srtc, RTC_RD_TIME, &rtc_tm);
	if (retval == -1) {
		perror("Error! RTC_RD_TIME");
		exit(errno);
	}

	/* Now, let's read the whole 47-bit counter value */
	retval = ioctl(fd_srtc, RTC_READ_TIME_47BIT, &time_47bit);
	if (retval == -1) {
		perror("Error! RTC_READ_TIME_47BIT");
		exit(errno);
	}

	/* fork a child process that will call the ioctl that waits for time set */
	pid_t pID = fork();

	/* child process */
	if (pID == 0) {

		/* set nice value to something higher than 0 (parent's nice value) */
		setpriority(PRIO_PROCESS, 0, CHILD_PROC_NICE_VAL);
		/* sleep for 1 second to ensure parent can run until it blocks waiting
		  * for time to be changed */
		sleep(1);

		/* Set the time to 16 sec ahead (actually 17 since we have already slept
		  * for 1 sec), and check for rollover of min & hr.
		  * Does not check for rollover of days, so this test may fail
		  * when run with RTC time = 11:59 PM */
		rtc_tm.tm_sec += 17;
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


		fprintf(stderr, "Child process: Trying to set RTC date/time ahead to %d-%d-%d, %02d:%02d:%02d.\n",
			rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
			rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

		retval = ioctl(fd_srtc, RTC_SET_TIME, &rtc_tm);
		if (retval == -1) {
			perror("Child process: Error! RTC_SET_TIME");
			exit(errno);
		}

		/* check to see if time was indeed set ahead by close to 16 seconds */
		retval = ioctl(fd_srtc, RTC_RD_TIME, &rtc_tm);
		if (retval == -1) {
			perror("Child process: Error! RTC_RD_TIME");
			exit(errno);
		}

		fprintf(stderr, "Child process: Current RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",
			rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
			rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);


		/* Set the time to 16 sec back, and check for rollover of min & hr.
		 * Does not check for rollover of days, so this test may fail
		 * when run with RTC time = 12:00 AM */
		if (rtc_tm.tm_sec > 15)
			rtc_tm.tm_sec -= 16;
		else
			rtc_tm.tm_sec = 44 + rtc_tm.tm_sec;
		if (rtc_tm.tm_sec >= 44) {
			if (rtc_tm.tm_min != 0)
				rtc_tm.tm_min--;
			else
				rtc_tm.tm_min = 59;
		}
		if (rtc_tm.tm_min == 59 && rtc_tm.tm_sec >= 44) {
			if (rtc_tm.tm_hour != 0)
				rtc_tm.tm_hour--;
			else
				rtc_tm.tm_hour = 23;
		}

		fprintf(stderr, "Child process: Trying to set RTC date/time back to %d-%d-%d, %02d:%02d:%02d.\n",
			rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
			rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

		retval = ioctl(fd_srtc, RTC_SET_TIME, &rtc_tm);
		if (retval == -1) {
			perror("Child process: Error! RTC_SET_TIME");
			exit(errno);
		}

		/* check to see if time was indeed set back by close to 16 seconds */
		retval = ioctl(fd_srtc, RTC_RD_TIME, &rtc_tm);
		if (retval == -1) {
			perror("Child process: Error! RTC_RD_TIME");
			exit(errno);
		}

		fprintf(stderr, "Child process: Current RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",
			rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
			rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);


		exit(0);

	}
	/* parent process */
	else if (pID > 0) {

		/* this is a blocking call */
		retval = ioctl(fd_srtc, RTC_WAIT_TIME_SET, &time_diff);
		if (retval == -1) {
			perror("Parent process: Error! RTC_WAIT_TIME_SET");
			exit(errno);
		}

		/* signed value means add 1 second */
		time_diff += (1 << 15);

		fprintf(stderr, "Parent process: time_diff = 0x%llx = %d seconds\n", time_diff, (int) (time_diff >> 15));

		/* The difference should be +16 seconds */
		if ((time_diff >> 15) != 16) {
			fprintf(stderr, "Parent process: Error! time_diff was not reported as 16 seconds\n");
			exit(1);
		}

		/* this is a blocking call */
		retval = ioctl(fd_srtc, RTC_WAIT_TIME_SET, &time_diff);
		if (retval == -1) {
			perror("Parent process: Error! RTC_WAIT_TIME_SET");
			exit(errno);
		}

		/* signed value means add 1 second */
		time_diff += (1 << 15);

		fprintf(stderr, "Parent process: time_diff = 0x%llx = %d seconds\n", time_diff, 	(int) (time_diff >> 15));

		/* The difference should be -16 seconds */
		if ((time_diff >> 15) != -16) {
			fprintf(stderr, "Parent process: Error! time_diff was not reported as -16 seconds\n");
			exit(1);
		}

		/* wait for child to complete before exiting, and get its return value */
		wait(&retval);
		retval = WEXITSTATUS(retval);
	}
	/* error during fork */
	else {
		perror("Error: Could not fork a child");
		close(fd_srtc);
		exit(errno);
	}

	close(fd_srtc);
	return retval;
}

