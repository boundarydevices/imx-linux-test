/* 
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved. 
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
 * @file   pmic_adc_main.c
 * @brief  PMIC ADC test main source file
 */

/*==============================================================================
Total Tests: PMIC adc

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
#include "pmic_adc_test.h"
#include "pmic_adc_test_TS.h"
#include "pmic_adc_test_read.h"
#include "pmic_adc_test_monitor.h"
#include "pmic_adc_test_convert.h"
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

char *ch_test_case;  /* test case */

/*==============================================================================
                                       GLOBAL CONSTANTS
==============================================================================*/

/*==============================================================================
                                       GLOBAL VARIABLES
==============================================================================*/
/* Extern Global Variables */
extern int  Tst_count;              /* counter for tst_xxx routines.         */
extern char *TESTDIR;               /* temporary dir created by tst_tmpdir() */

/* Global Variables */
int fd_adc = 0;
char *TCID     = "pmic_testapp_adc";    /* test program identifier.          */
int  TST_TOTAL = 1;                  /* total number of tests in this file.   */

/*=============================================================================
                                   GLOBAL FUNCTION PROTOTYPES
==============================================================================*/
void cleanup(void);
void setup(void);
int main(int argc, char **argv);
void help(void);

/*==============================================================================
                                   LOCAL FUNCTION PROTOTYPES
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
void cleanup(void)
{
        int VT_rv = TFAIL;

        if(!strcmp(ch_test_case,TEST_CASE_TS))
        {
                VT_rv = VT_pmic_adc_TS_cleanup(); 
        } else if (!strcmp(ch_test_case,TEST_CASE_R))
        {
                VT_rv = VT_pmic_adc_read_cleanup(); 
        } else if (!strcmp(ch_test_case,TEST_CASE_CONV))
        {
                VT_rv = VT_pmic_adc_convert_cleanup(); 
        } else if (!strcmp(ch_test_case,TEST_CASE_MON))
        {
                VT_rv = VT_pmic_adc_monitor_cleanup(); 
        }  else 

        if ((fd_adc > 0) & (close(fd_adc) < 0))
        {
                        tst_resm(TWARN, "Unable to close file descriptor %d\n",
        			fd_adc);
                        VT_rv = TFAIL;
        }

        if (VT_rv != TPASS)
        {
                tst_resm(TWARN, "VT_cleanup() Failed : error code = %d\n", 
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
void setup(void)
{
        if ((fd_adc = open(PMIC_DEVICE_ADC, O_RDWR)) < 0)
        {
                tst_brkm(TBROK , cleanup, "Unable to open \n", PMIC_DEVICE_ADC);
        }

        int VT_rv = TFAIL;

        if(!strcmp(ch_test_case,TEST_CASE_TS))
        {
                VT_rv = VT_pmic_adc_TS_setup(); 
        } else if (!strcmp(ch_test_case,TEST_CASE_R))
        {
                VT_rv = VT_pmic_adc_read_setup(); 
        } else if (!strcmp(ch_test_case,TEST_CASE_CONV))
        {
                VT_rv = VT_pmic_adc_convert_setup(); 
        } else if (!strcmp(ch_test_case,TEST_CASE_MON))
        {
                VT_rv = VT_pmic_adc_monitor_setup(); 
        } else 
        {
                tst_brkm(TBROK , tst_exit, "Not correct parametres");
        }

        if (VT_rv != TPASS)
        {
                tst_brkm(TBROK , cleanup, "VT_setup() Failed : "
        		"error code = %d\n", VT_rv);
        }
}

/*============================================================================*/
/*===== main =====*/
/**
@brief  Entry point to this test-case. It parses all the command line
        inputs, calls the global setup and executes the test. It logs
        the test status and results appropriately using the LTP API's
        On successful completion or premature failure, cleanup() function
        is called and test exits with an appropriate return code.

@param  Input :      argc - number of command line parameters.
        Output:      **argv - pointer to the array of the command line parameters.
        -T  test_case - exec test case by prefix
                test cases prefixes:
                TS - Test touch screen and convert function
                read - Test read interface
                CONV - Test touch screen and convert function correctness
  
@return On failure - Exits by calling cleanup().
        On success - exits with 0 exit value.
*/
/*============================================================================*/
int main(int argc, char **argv)
{
        int VT_rv = TFAIL;
        
        /* parse options. */
        int tflag=0;                 /* binary flags: opt or not */
        char *msg;
        
        option_t options[] =
        {
                { "T:", &tflag, &ch_test_case  },/* Test case */
                { NULL, NULL, NULL }             /* NULL required to 
                                                    end array */
        };
        
        if ( (msg=parse_opts(argc, argv, options, &help)) != NULL )
        {
                tst_brkm(TBROK, tst_exit, "Not correct params\n", TCID);
        }

        /* Test Case Body. */
        if(tflag) {
                /* Print test Assertion using tst_resm() function 
                   with argument TINFO. */
                tst_resm(TINFO, "Testing if %d test case is OK\n", 
        		TCID,ch_test_case);
                setup();
                if(!strcmp(ch_test_case,TEST_CASE_TS))
                {
                        VT_rv = VT_pmic_adc_test_TS(); 
                } else if (!strcmp(ch_test_case,TEST_CASE_R))
                {
                        VT_rv = VT_pmic_adc_test_read(); 
                } else if (!strcmp(ch_test_case,TEST_CASE_CONV))
                {
                        VT_rv = VT_pmic_adc_test_convert(); 
                } else if (!strcmp(ch_test_case,TEST_CASE_MON))
                {
                        VT_rv = VT_pmic_adc_test_monitor(); 
                }

                if(VT_rv == TPASS)
                {
                        tst_resm(TPASS, "%d test case worked as expected\n", 
        		TCID,ch_test_case);
                } else
                {
                        tst_resm(TFAIL, "%d test case did NOT work as "
        			"expected\n", TCID,ch_test_case);
                }
                cleanup();
        } else
        {
                help();
        }

        return VT_rv;
}

void help(void)
{
        printf("PMIC adc driver option\n");
        printf("  -T test case 'TS', 'read', 'CONV', 'MON'\n");
}
