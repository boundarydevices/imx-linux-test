/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file   mc13783_test_D.c
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
#include "mc13783_test_common.h"
#include "mc13783_test_D.h"

/*==============================================================================
                                        LOCAL MACROS
==============================================================================*/

/*==============================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==============================================================================*/

/*==============================================================================
                                       LOCAL CONSTANTS
==============================================================================*/

#define nb_value 3
	unsigned int TEST_VALUE_D[nb_value][3] = {
		{REG_INTERRUPT_MASK_0, 0x555555, FALSE},
		{REG_INTERRUPT_MASK_1, 0x123456, FALSE},
		{REG_USB, 0xFF7777, FALSE}
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
/*===== VT_mc13783_setup =====*/
/**
@brief  assumes the initial condition of the test case execution

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc13783_D_setup(void) {
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
	int VT_mc13783_D_cleanup(void) {
		int rv = TFAIL;

		rv = TPASS;

		return rv;
	}

/*============================================================================*/
/*===== VT_mc13783_test_D =====*/
/**
@brief  MC13783 test scenario Register dependencies

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc13783_test_D(void) {
		int rv = TPASS, fd, i = 0;
		unsigned int val;

		fd = open(MC13783_DEVICE, O_RDWR);
		if (fd < 0) {
			pthread_mutex_lock(&mutex);
			tst_resm(TFAIL, "Unable to open %s", MC13783_DEVICE);
			pthread_mutex_unlock(&mutex);
			return TFAIL;
		}

		for (i = 0; i < nb_value; i++) {
			if (VT_mc13783_opt(fd, CMD_WRITE, TEST_VALUE_D[i][0],
					   &(TEST_VALUE_D[i][1])) != TPASS) {
				rv = TFAIL;
				TEST_VALUE_D[i][2] = TRUE;
			} else {
				if (VT_mc13783_opt
				    (fd, CMD_READ, TEST_VALUE_D[i][0],
				     &val) != TPASS) {
					rv = TFAIL;
					TEST_VALUE_D[i][2] = TRUE;
				} else if (val != TEST_VALUE_D[i][1]) {
					pthread_mutex_lock(&mutex);
					tst_resm(TFAIL,
						 "Value 0x%X haven't been writed"
						 " to reg %d",
						 TEST_VALUE_D[i][1],
						 TEST_VALUE_D[i][0]);
					pthread_mutex_unlock(&mutex);
					TEST_VALUE_D[i][2] = TRUE;
					rv = TFAIL;
				}
			}
		}

		for (i = 0; i < nb_value; i++) {
			if (TEST_VALUE_D[i][2] == FALSE) {
				if (VT_mc13783_opt
				    (fd, CMD_READ, TEST_VALUE_D[i][0],
				     &val) != TPASS) {
					rv = TFAIL;
				} else if (val != TEST_VALUE_D[i][1]) {
					pthread_mutex_lock(&mutex);
					tst_resm(TFAIL,
						 "Value 0x%X haven't been "
						 "changed since writing to reg %d",
						 val, TEST_VALUE_D[i][0]);
					pthread_mutex_unlock(&mutex);
					rv = TFAIL;
				}
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
