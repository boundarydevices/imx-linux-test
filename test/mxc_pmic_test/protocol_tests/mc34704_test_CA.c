/*
 * Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file   mc34704_test_CA.c
 * @brief  Test scenario C source PMIC.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
                                        INCLUDE FILES
==============================================================================*/
/* Standard Include Files */
#include <errno.h>

/* Harness Specific Include Files. */
#include "../include/test.h"

/* Verification Test Environment Include Files */
#include "mc34704_test_common.h"
#include "mc34704_test_CA.h"

/*==============================================================================
                                        LOCAL MACROS
==============================================================================*/

/*==============================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==============================================================================*/

/*==============================================================================
                                       LOCAL CONSTANTS
==============================================================================*/

#define nb_value 8

	unsigned int TEST_TMP;
	unsigned int TEST_VALUE_CA[nb_value][3] = {
		{CMD_READ, REG_MC34704_GENERAL1, &TEST_TMP},
		{CMD_SUB, 2, 0},
		{CMD_READ, REG_MC34704_GENERAL2, &TEST_TMP},
		{CMD_SUB, 0, 0},
		{CMD_READ, REG_MC34704_FAULTS, &TEST_TMP},
		{CMD_UNSUB, 0, 0},
		{CMD_UNSUB, 2, 0}
	};

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
/*===== VT_mc34704_setup =====*/
/**
@brief  assumes the initial condition of the test case execution

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc34704_CA_setup(void) {
		int rv = TFAIL;

		rv = TPASS;

		return rv;
	}

/*============================================================================*/
/*===== VT_cleanup =====*/
/**
@brief  assumes the post-condition of the test case execution

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc34704_CA_cleanup(void) {
		int rv = TFAIL;

		rv = TPASS;

		return rv;
	}

/*============================================================================*/
/*===== VT_mc34704_test_CA =====*/
/**
@brief  MC34704 test scenario Concurrent Access

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc34704_test_CA(void) {
		int rv = TPASS, fd, i = 0;

		fd = open(MC34704_DEVICE, O_RDWR);
		if (fd < 0) {
			pthread_mutex_lock(&mutex);
			tst_resm(TFAIL, "Unable to open %s", MC34704_DEVICE);
			pthread_mutex_unlock(&mutex);
			return TFAIL;
		}

		for (i = 0; i < nb_value; i++) {
			if (VT_mc34704_opt
			    (fd, TEST_VALUE_CA[i][0], TEST_VALUE_CA[i][1],
			     &(TEST_VALUE_CA[i][2])) != TPASS) {
				rv = TFAIL;
			}
		}

		if (close(fd) < 0) {
			pthread_mutex_lock(&mutex);
			tst_resm(TFAIL, "Unable to close file descriptor %d",
				 fd);
			pthread_mutex_unlock(&mutex);
			return TFAIL;
		}

		return rv;
	}

#ifdef __cplusplus
}
#endif
