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

#define MAX_INSTANCES 4

int main(int ac, char *av[])
{
	int fd_audio, i, j, tmp, err, toterr, inst_num;
	char drv_name[32];

	printf("Hi... \n");

	if (ac == 1) {
		printf
		    ("This program checks that it is possible to open only once a driver instance\n");
		printf("-> main <supported_driver_instance_number>\n\n");
		return 0;
	}

	inst_num = atoi(av[1]);
	if (inst_num > MAX_INSTANCES) {
		printf("Sounds like too many instances.\n");
		inst_num = MAX_INSTANCES;
		printf
		    ("Will check till #%i. Change limitation in source and recompile if needed...\n",
		     inst_num);
	}

	for (i = 0, toterr = 0; i < inst_num; i++) {
		if (i == 0)
			sprintf(drv_name, "/dev/sound/dsp");
		else
			sprintf(drv_name, "/dev/sound/dsp%i", i);

		for (j = 0, err = 0; j < 3; j++) {
			if ((tmp = open(drv_name, O_WRONLY)) < 0) {
	      /** did not succeeded to open the driver */
				if (j == 0) {
					printf("Error at first opening %s\n",
					       drv_name);
					err++;
					break;	// exit for this instance
				}
			} else {
	      /** succeeded to open the driver */
				if (j == 0)
					fd_audio = tmp;
				else {
					printf("Error : driver re-opened\n");
					close(tmp);
					err++;
					break;	// exit for this instance
				}
			}
		}

		close(fd_audio);	// close the only opened instance
		if (!err)
			printf("Test passed successfully for %s\n", drv_name);
		toterr += err;
	}

	if (!toterr)
		printf("\n --> All the tests were passed successfully\n");
	else
		printf("\nEncountered %i errors\n", toterr);

	return 0;
}
