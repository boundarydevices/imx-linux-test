/*
 * Copyright 2005-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file   pmic_battery_main.c
 * @brief  PMIC Battery test main source file
 */

/*==============================================================================
Total Tests: PMIC battery

Test Name: charger, eol, led, reverse supply and unregulated tests

Test Assertion
& Strategy: This test is using to test charger, eol, led, reverse supply and unregulated functions
of sc55112 Battery driver.
==============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
                                        INCLUDE FILES
==============================================================================*/
/* Standard Include Files */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* Harness Specific Include Files. */
#include "test.h"
#include "usctest.h"

/* Verification Test Environment Include Files */
#include "pmic_battery_test.h"

/*==============================================================================
                                       GLOBAL VARIABLES
==============================================================================*/
	char *TCID = "pmic_battery_testapp";	/* test program identifier. */
	int fd;			/* PMIC test device descriptor */
	int TST_TOTAL = 4;	/* total number of tests in this file. */

/*==============================================================================
                                   GLOBAL FUNCTION PROTOTYPES
==============================================================================*/
	int main(int argc, char **argv);
	void help(void);

/*==============================================================================
                                   LOCAL FUNCTIONS
==============================================================================*/
/*============================================================================*/
/*===== cleanup =====*/
/**
@brief  This function assumes the post-condition of the test case execution

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	void cleanup(void) {
		int ret;
		 ret = close(fd);
		if (ret < 0) {
			tst_resm(TWARN, "Unable to close file descriptor %d, "
				 "error code %d", fd, ret);
		};
		tst_exit();
	}
/*============================================================================*/
/*===== setup =====*/
/*
@brief  This function assumes the setup condition of the test case execution

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int setup(void) {
		fd = open("/dev/" PMIC_BATT_DEV, O_RDWR);
		if (fd < 0) {
			tst_brkm(TBROK, cleanup, "Unable to open %s\n",
				 PMIC_BATT_DEV);
		};
		return TPASS;
	}
/*============================================================================*/
/*===== main =====*/
/*
@brief  Entry point to this test-case. It parses all the command line
        inputs, calls the global setup and executes the test. It logs
        the test status and results appropriately using the LTP API's
        On successful completion or premature failure, cleanup() function
        is called and test exits with an appropriate return code.

@param  Input :      argc - number of command line parameters.
        Output:      **argv - pointer to the array of the command
        		      line parameters.

@return On failure - Exits by calling cleanup().
        On success - exits with 0 exit value.
*/
/*============================================================================*/
	int main(int argc, char **argv) {
		int VT_rv = TFAIL;
		int change_test_case = 0;

		/* parse options. */
		int tflag = 0;	/* binary flags: opt or not */
		char *ch_test_case = 0;	/* option arguments */
		char *msg = 0;

		option_t options[] = {
			{"T:", &tflag, &ch_test_case},	/* argument required */
			{NULL, NULL, NULL}	/* NULL required to
						   end array */
		};

		if ((msg = parse_opts(argc, argv, options, &help)) != NULL) {
			tst_resm(TWARN, "%s test did not work as expected. "
				 "Parse options error: %s", TCID, msg);
			help();
			return VT_rv;
		}

		/* Test Case Body. */
		if (tflag == 0) {
			/* Print results and exit test scenario */
			help();
			cleanup();
			return VT_rv;
		}

		setup();

		change_test_case = atoi(ch_test_case);

		if (change_test_case >= 0 && change_test_case < 10) {
			VT_rv = VT_pmic_batt_test(change_test_case);
			tst_resm(TINFO, "Testing %s_%s test case is OK", TCID,
				 ch_test_case);
			if (VT_rv == TPASS) {
				tst_resm(TPASS,
					 "%s_%s test case working as expected",
					 TCID, ch_test_case);
			} else {
				tst_resm(TFAIL,
					 "%s_%s test case didn't work as "
					 "expected", TCID, ch_test_case);
			};
			cleanup();
		} else {
			VT_rv = TFAIL;
			tst_resm(TFAIL,
				 "Invalid arg for -T: %d. OPTION VALUE OUT OF "
				 "RANGE", change_test_case);
			help();
		};

		cleanup();

		return VT_rv;
	}
/*============================================================================*/
	void help(void) {
		printf("\n===========================================\n");
		printf("PMIC battery driver option\n");
		printf("\t  '-T 0'   Test charger management\n");
		printf("\t  '-T 1'   Test eol comparator\n");
		printf("\t  '-T 2'   Test led management\n");
		printf
		    ("\t  '-T 3'   Test reverse supply and unregulated modes\n");
		printf("\t  '-T 4'   Test set out control\n");
		printf("\t  '-T 5'   Test set over voltage threshold\n");
		printf("\t  '-T 6'   Test get charger current\n");
		printf("\t  '-T 7'   Test get battery voltage\n");
		printf("\t  '-T 8'   Test get battery current\n");
		printf("\t  '-T 9'   Test get charger voltage\n");
	}

#ifdef __cplusplus
}
#endif
