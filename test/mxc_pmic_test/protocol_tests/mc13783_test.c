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
 * @file   mc13783_test.c
 * @brief  Test scenario C source MC13783.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
                                        INCLUDE FILES
==============================================================================*/
/* Standard Include Files */

/* Harness Specific Include Files. */
#include "../include/test.h"

/* Verification Test Environment Include Files */
#include "mc13783_test.h"
#include "mc13783_test_common.h"
#include "mc13783_test_RW.h"
#include "mc13783_test_SU.h"
#include "mc13783_test_S_IT_U.h"
#include "mc13783_test_D.h"
#include "mc13783_test_OC.h"
#include "mc13783_test_CA.h"
#include "mc13783_test_RA.h"
#include "mc13783_test_IP.h"

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

/*==============================================================================
                                   LOCAL FUNCTION PROTOTYPES
==============================================================================*/

/*==============================================================================
                                       LOCAL FUNCTIONS
==============================================================================*/

/*============================================================================*/
/*===== VT_mc13783_exec_opt =====*/
/**
@brief  open MC13783, perform operation on it and then close it.

@param  Input :		params - pointer to opt_params structure, 
                        operation - performing operation 
                        	(CMD_READ, CMD_WRITE, CMD_SUB, CMD_UNSUB)
                        val1 - reg number if operation is CMD_READ or CMD_WRITE,
                                reg number if operation is CMD_READ 
                                or CMD_WRITE,
                        *val2 - writing reg value in case of CMD_WRITE operation
        Output:         *val2 - return reg value in case of CMD_READ operation

@return On success - return TPASS
        On failure - return TFAIL

*/
/*============================================================================*/

	void *VT_mc13783_exec_opt(void *params) {
		int fd, rv;

		 fd = open(MC13783_DEVICE, O_RDWR);
		if (fd < 0) {
			tst_resm(TFAIL, "Unable to open %s\n", MC13783_DEVICE);
			return (void *)TFAIL;
		}

		opt_params optparams = *((opt_params *) params);

		rv = VT_mc13783_opt(fd, optparams.operation, optparams.val1,
				    optparams.val2);
		if (rv != 0) {
			close(fd);
			return (void *)TFAIL;
		}

		rv = close(fd);
		if (rv < 0) {
			tst_resm(TFAIL, "Unable to close file descriptor %d\n",
				 fd);
			return (void *)TFAIL;
		}

		return (void *)TPASS;
	}

/*============================================================================*/
/*===== VT_mc13783_exec_test_case =====*/
/**
@brief  exec MC13783 test scenario

@param  Input :                *test_case - Test case prefix
        Output:                None.

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	void *VT_mc13783_exec_test_case(void *test_case) {
		int VT_rv = TFAIL;

		if (!strcmp((char *)test_case, TEST_CASE_RW)) {
			VT_rv = VT_mc13783_test_RW();
		} else if (!strcmp((char *)test_case, TEST_CASE_SU)) {
			VT_rv = VT_mc13783_test_SU();
		} else if (!strcmp((char *)test_case, TEST_CASE_S_IT_U)) {
			VT_rv = VT_mc13783_test_S_IT_U();
		} else if (!strcmp((char *)test_case, TEST_CASE_D)) {
			VT_rv = VT_mc13783_test_D();
		} else if (!strcmp((char *)test_case, TEST_CASE_OC)) {
			VT_rv = VT_mc13783_test_OC();
		} else if (!strcmp((char *)test_case, TEST_CASE_CA)) {
			VT_rv = VT_mc13783_test_CA();
		} else if (!strcmp((char *)test_case, TEST_CASE_RA)) {
			VT_rv = VT_mc13783_test_RA();
		} else if (!strcmp((char *)test_case, TEST_CASE_IP)) {
			VT_rv = VT_mc13783_test_IP();
		}

		return (void *)VT_rv;
	}

#ifdef __cplusplus
}
#endif
