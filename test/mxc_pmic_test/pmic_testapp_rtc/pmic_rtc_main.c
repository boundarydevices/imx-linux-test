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
 * @file   pmic_rtc_main.c
 * @brief  PMIC RTC test main source file.
 */

/*==============================================================================
Total Tests: PMIC rtc

Test Name:   Read/Write/Event test

Test Assertion
& Strategy:  This test is used to test the PMIC protocol.
==============================================================================*/

#ifdef __cplusplus
extern "C"{
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
#include "pmic_rtc_test.h"

/*==============================================================================
                                        LOCAL MACROS
==============================================================================*/

/*==============================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==============================================================================*/

/*==============================================================================
                                       LOCAL CONSTANTS
==============================================================================*/
#if !defined(TRUE) && !defined(FALSE)
#define TRUE  1
#define FALSE 0
#endif

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
extern int  Tst_count;               /* counter for tst_xxx routines.         */
extern char *TESTDIR;                /* temporary dir created by tst_tmpdir() */

/* Global Variables */
char *TCID     = "PMIC_TestApp_rtc"; /* test program identifier.          */
int  TST_TOTAL = 3;                  /* total number of tests in this file.   */

/*==============================================================================
                                   GLOBAL FUNCTION PROTOTYPES
==============================================================================*/

int main(int argc, char **argv);
void help(void);
void cleanup(void);

/*==============================================================================
                                       LOCAL FUNCTIONS
==============================================================================*/

/*============================================================================*/
/*===== main =====*/
/**
@brief  Entry point to this test-case. It parses all the command line
        inputs, calls the global setup and executes the test. It logs
        the test status and results appropriately using the LTP API's
        On successful completion or premature failure, cleanup() function
        is called and test exits with an appropriate return code.

@param  Input : argc - number of command line parameters.
        Output: **argv - pointer to the array of the command line parameters.
                Describe input arguments to this test-case
                -l - Number of iteration
                -v - Prints verbose output
                -V - Prints the version number

@return On failure - Exits by calling cleanup().
        On success - exits with 0 exit value.
*/
/*============================================================================*/

void cleanup(void)
{
        /* Exit with appropriate return code. */
        tst_exit();
}

int main(int argc, char **argv)
{
        int VT_rv = TFAIL;

        /* parse options. */
        int tflag=0;                 /* binary flags: opt or not */
        char *ch_test_case;  /* option arguments */
        char *msg;

        option_t options[] = {
                { "T:", &tflag, &ch_test_case  },       /* argument required */
                { NULL, NULL, NULL }           /* NULL required to end array */
        };

        if ( (msg=parse_opts(argc, argv, options, &help)) != NULL ) {
                tst_brkm(TBROK , cleanup,
                        "%s test case did NOT work as expected\n", VT_rv);
        }

        /* Test Case Body. */
        if(tflag) {
                /* Print test Assertion using tst_resm() function with
                   argument TINFO. */
                tst_resm(TINFO, "Testing if %s_%s test case is OK\n", TCID,
                        ch_test_case);

                if(!strcmp(ch_test_case,TEST_CASE1)) {
                        VT_rv = VT_pmic_rtc_test(0);
                } else if(!strcmp(ch_test_case,TEST_CASE2)) {
                        VT_rv = VT_pmic_rtc_test(1);
                } else if(!strcmp(ch_test_case,TEST_CASE3)) {
                        VT_rv = VT_pmic_rtc_test(2);
                } else if(!strcmp(ch_test_case,TEST_CASE4)) {
                        VT_rv = VT_pmic_rtc_test(3);
                } else if(!strcmp(ch_test_case,TEST_CASE5)) {
                        VT_rv = VT_pmic_rtc_test(4);
                }

                if(VT_rv == TPASS) {
                        tst_resm(TPASS, "%s_%s test case worked as expected\n",
                                TCID,ch_test_case);
                } else {
                        tst_resm(TFAIL, "%s_%s test case did NOT work as "
                                "expected\n", TCID,ch_test_case);
                }
        }

        if(tflag==0) {
                /* VTE : print results and exit test scenario */
                help();
        }
        return VT_rv;
}

void help(void)
{
        printf("Pmic rtc driver option\n");
        printf("  -T TEST       Test all Pmic rtc\n");
        printf("  -T TIME       Test time and date read and write RTC functions\n");
        printf("  -T ALARM      Test alarm read and write RTC functions\n");
        printf("  -T WAIT_ALARM Test Alarm IT function\n");
        printf("  -T POLL_TEST  Test Alarm IT poll function\n");
}

