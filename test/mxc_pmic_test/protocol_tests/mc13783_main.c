/* 
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved. 
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
 * @file   mc13783_main.c
 * @brief  MC13783 Protocol driver testing.
 */

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
#include <pthread.h>

/* Harness Specific Include Files. */
#include "../include/usctest.h"
#include "../include/test.h"

/* Verification Test Environment Include Files */
#include "mc13783_test_common.h"
#include "mc13783_test.h"
/*==============================================================================
                                        LOCAL MACROS
==============================================================================*/

/*==============================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==============================================================================*/

/*==============================================================================
                                       LOCAL CONSTANTS
==============================================================================*/

/*==============================================================================
                                       LOCAL VARIABLES
==============================================================================*/

/*==============================================================================
                                       GLOBAL CONSTANTS
==============================================================================*/

/*==============================================================================
                                       GLOBAL VARIABLES
==============================================================================*/
/* Extern Global Variables */
	extern int Tst_count;	/* counter for tst_xxx routines.         */
	extern char *TESTDIR;	/* temporary dir created by tst_tmpdir() */

/* Global Variables */
	char *TCID = "mc13783_testapp";	/* test program identifier.          */
	int TST_TOTAL = 8;	/* total number of tests in this file.   */

	int threads_num = 1;
	int *thr_result;
	pthread_t *thread;

	int verbose_flag;
	pthread_mutex_t mutex;
/*==============================================================================
                                   GLOBAL FUNCTION PROTOTYPES
==============================================================================*/
	void cleanup(void);
	void setup(void);
	int main(int argc, char **argv);
	void help(void);

/*==============================================================================
                                   LOCAL FUNCTION PROTOTYPES
==============================================================================*/

/*==============================================================================
                                       GLOBAL FUNCTIONS
==============================================================================*/

/*============================================================================*/
/*===== cleanup =====*/
/**
@brief  Performs all one time clean up for this test on successful
                completion,  premature exit or  failure. Closes all temporary
                files, removes all temporary directories exits the test with
                appropriate return code by calling tst_exit() function.cleanup

@param  Input :      None.
        Output:      None.
  
@return Nothing
*/
/*============================================================================*/
	void cleanup(void) {
		/* VTE : Actions needed to get a stable target environment */
		int VT_rv = TFAIL;

		 free(thr_result);
		 free(thread);

		 VT_rv = VT_mc13783_cleanup();
		if (VT_rv != TPASS) {
			tst_resm(TWARN,
				 "VT_cleanup() Failed : error code = %d\n",
				 VT_rv);
		}

		tst_exit();
	}

/*==============================================================================
                                       LOCAL FUNCTIONS
==============================================================================*/

/*============================================================================*/
/*===== setup =====*/
/**
@brief  Performs all one time setup for this test. This function is
        typically used to capture signals, create temporary directories
        and temporary files that may be used in the course of this test.

@param  Input :      None.
        Output:      None.
  
@return On failure - Exits by calling cleanup().
        On success - returns 0.
*/
/*============================================================================*/
	void setup(void) {
		int VT_rv = TFAIL;

		/* VTE : Actions needed to prepare the test running */
		if ((thr_result =
		     malloc(sizeof(opt_params) * threads_num)) == NULL) {
			tst_brkm(TBROK, cleanup,
				 "VT_setup() Failed : error code = " "%d\n",
				 VT_rv);
		}
		if ((thread = malloc(sizeof(pthread_t) * threads_num)) == NULL) {
			tst_brkm(TBROK, cleanup,
				 "VT_setup() Failed : error code = " "%d\n",
				 VT_rv);

		}
		VT_rv = VT_mc13783_setup();
		if (VT_rv != TPASS) {
			tst_brkm(TBROK, cleanup,
				 "VT_setup() Failed : error code = " "%d\n",
				 VT_rv);
		}
		/* VTE */

	}

/*============================================================================*/
/*===== main =====*/
/**
@brief  Entry point to this test-case. It parses all the command line
        inputs, calls the global setup and executes the test. It logs
        the test status and results appropriately using the LTP API's
        On successful completion or premature failure, cleanup() func
        is called and test exits with an appropriate return code.

@param  Input :    argc - number of command line parameters.
        Output:    **argv - pointer to the array of the command line parameters.
        Describe input arguments to this test-case
         -v - Print verbose output
         -c - Number of concurrent threads
         -R reg_number - Read from register reg_number
         -W reg_number - Write to register reg_number
         -S event_number - Subscribe event event_number
         -U event_number - Unsubscribe event event_number
         -T  test_case - exec test case by prefix
             test cases prefixes:
             RW - Read/write test
             SU - Subscribe/Unsubscribe test
             S_IT_U - Subscribe/interrupt/Unsubscribe test
             D - Register dependencies test
             OC - Open/close test
             CA - Concurrent access test
             RA - Random access test
             IP - Access with incorrect parameters test

@return On failure - Exits by calling cleanup().
        On success - exits with 0 exit value.
*/
/*============================================================================*/
	int main(int argc, char **argv) {
		int VT_rv = TPASS;

		/* parse options. */
		/* binary flags: opt or not */
		int rflag = 0, wflag = 0, vflag = 0, sflag = 0, uflag =
		    0, tflag = 0;
		/* option arguments */
		char *ch_reg_num, *ch_reg_value, *ch_event_num, *ch_test_case;
		/* option arguments */
		unsigned int reg_num = 0, reg_value = 0, event_num = 0;
		char *msg;
		opt_params optparams;
		void *params;
		void *(*func) (void *) = NULL;

		option_t options[] = {
			{"R:", &rflag, &ch_reg_num},	/* Read register opt */
			{"W:", &wflag, &ch_reg_num},	/* Write register opt */
			{"V:", &vflag, &ch_reg_value},	/* Value to write */
			{"S:", &sflag, &ch_event_num},	/* Subscribe event opt */
			{"U:", &uflag, &ch_event_num},	/* Unsubscribe event opt */
			{"T:", &tflag, &ch_test_case},	/* Execute test case */
			{"v", &verbose_flag, NULL},	/* Verbose flag */
			{NULL, NULL, NULL}	/* NULL required to 
						   end array */
		};

		if ((msg = parse_opts(argc, argv, options, &help)) != NULL) {
			tst_resm(TFAIL, "%s\n", msg);
			tst_resm(TFAIL,
				 "%s test case did NOT work as expected\n",
				 TCID);
			return VT_rv;
		}

		threads_num = STD_COPIES;

		if (rflag || wflag) {
			reg_num = atoi(ch_reg_num);
		}
		if (sflag || uflag) {
			event_num = atoi(ch_event_num);
		}
		if (vflag) {
			sscanf((char *)ch_reg_value, "%x", &reg_value);
		}

		/* perform global test setup, call setup() function. */
		setup();

		/* Test Case Body. */

		if (tflag) {
			/* Print test Assertion using tst_resm() function 
			   with argument TINFO. */
			func = &VT_mc13783_exec_test_case;
			params = (void *)ch_test_case;
#ifdef __CVS_TEST
			VT_rv = (int)VT_mc13783_exec_test_case(params);
#endif
		} else {
			if (rflag) {
				optparams.operation = CMD_READ;
				optparams.val1 = reg_num;
				optparams.val2 = &reg_value;
				ch_test_case = "R";
			} else if (wflag) {
				optparams.operation = CMD_WRITE;
				optparams.val1 = reg_num;
				optparams.val2 = &reg_value;
				ch_test_case = "W";
			} else if (sflag) {
				optparams.operation = CMD_SUB;
				optparams.val1 = event_num;
				optparams.val2 = NULL;
				ch_test_case = "S";
			} else if (uflag) {
				optparams.operation = CMD_UNSUB;
				optparams.val1 = event_num;
				optparams.val2 = NULL;
				ch_test_case = "U";
			}
			func = &VT_mc13783_exec_opt;
			params = (void *)(&optparams);
#ifdef __CVS_TEST
			VT_rv = (int)VT_mc13783_exec_opt(&optparams);
#endif
		}

#ifndef __CVS_TEST
		int i;
		for (i = 0; i < threads_num; i++) {
			thr_result[i] = pthread_create(&(thread[i]), NULL,
						       func, params);
			if (thr_result[i] != 0) {
				if (verbose_flag) {
					tst_resm(TFAIL,
						 "Thread %d starting failed\n",
						 i);
				}
				VT_rv = TFAIL;
			} else if (verbose_flag) {
				tst_resm(TINFO, "Thread %d started\n", i);
			}
		}

		for (i = 0; i < threads_num; i++) {
			if (thr_result[i] == 0) {
				pthread_join(thread[i],
					     (void *)&(thr_result[i]));
				if (thr_result[i] == 0) {
					if (verbose_flag) {
						tst_resm(TINFO,
							 "Thread %d finished: "
							 "OK\n", i);
					}
				} else {
					if (verbose_flag) {
						tst_resm(TFAIL,
							 "Thread %d finished: "
							 "FAILED\n", i);
					}
				}
			}
		}

		for (i = 0; i < threads_num; i++)
			if (thr_result[i] != 0) {
				VT_rv = TFAIL;
			}
#endif
		/* Print test Assertion using tst_resm() */
		if (VT_rv == TPASS) {
			tst_resm(TPASS, "%s test worked as expected\n", TCID);
		} else {
			tst_resm(TFAIL, "%s test did NOT work as expected\n",
				 TCID);
		}

		cleanup();
		return VT_rv;
	}

	void help(void) {
		printf("MC13783 core driver option\n");
		printf("  -R <Reg>  Read a register\n");
		printf("  -W <Reg>  write a register\n");
		printf("  -V <hexa value> used for write option\n");
		printf("  -S <event_number> Subscribe event notification\n");
		printf("  -U <event_number> Unsubscribe event notification\n");
		printf("  -v display information\n");
		printf("\tWith event_number value is:\n");
		printf(" 0 : ADC has finished\t\t\t");
		printf(" 1 : ADCBIS has finished\n");
		printf(" 2 : Touchscreen wakeup\t\t\t");
		printf(" 3 : ADC reading above high limit\n");
		printf(" 4 : ADC reading below low limit\t");
		printf(" 5 : Charger attach and removal\n");
		printf(" 6 : Charger over-voltage detection\t");
		printf(" 7 : Charger path reverse current\n");
		printf(" 8 : Charger path short circuit\t\t");
		printf(" 9 : BP reg current or voltage\n");
		printf(" 10: Charge current below threshold\t");
		printf(" 11: BP turn on threshold detection\n");
		printf(" 12: End of life / low battery detect\t");
		printf(" 13: Low battery warning\n");
		printf(" 14: USB detect\t\t\t\t");
		printf(" 15: USB ID Line detect\n");
		printf(" 16: Single ended 1 detect\t\t");
		printf(" 17: Car-kit detect\n");
		printf(" 18: 1 Hz time-tick\t\t\t");
		printf(" 19: Time of day alarm\n");
		printf(" 20: ON1B event\t\t\t\t");
		printf(" 21: ON2B event\n");
		printf(" 22: ON3B event\t\t\t\t");
		printf(" 23: System reset\n");
		printf(" 24: RTC reset occurred\t\t\t");
		printf(" 25: Power cut event\n");
		printf(" 26: Warm start event\t\t\t");
		printf(" 27: Memory hold event\n");
		printf(" 28: Power ready\t\t\t");
		printf(" 29: Thermal warning lower threshold\n");
		printf(" 30: Thermal warning higher threshold\t");
		printf(" 31: Clock source change\n");
		printf(" 32: Semaphore\t\t\t\t");
		printf(" 33: Microphone bias 2 detect\n");
		printf(" 34: Headset attach\t\t\t");
		printf(" 35: Stereo headset detect\n");
		printf(" 36: Thermal shutdown ALSP\t\t");
		printf(" 37: Short circuit on AHS outputs\n");
#ifdef __CVS_TEST
		printf("  -T test case 'RW', 'SU', 'S_IT_U\n");
#else
		printf
		    ("  -T test case 'RW', 'SU', 'S_IT_U, 'D', 'OC', 'CA', 'RA',"
		     " 'IP'\n");
#endif
		printf("Example :\n");
		printf("\tmc13783_testapp -W 18 -V 05F4DA\n");
		printf("\tmc13783_testapp -v -R 18\n");
		printf("\tmc13783_testapp -S 20\n");
		printf("\tmc13783_testapp -U 20\n");
	}

#ifdef __cplusplus
}
#endif
