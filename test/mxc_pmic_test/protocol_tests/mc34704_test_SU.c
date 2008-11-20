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
 * @file   mc34704_test_SU.c
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
#include "mc34704_test_SU.h"

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
/*===== VT_mc34704_SU_setup =====*/
/**
@brief  assumes the initial condition of the test case execution

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc34704_SU_setup(void) {
		int rv = TFAIL;

		 rv = TPASS;

		 return rv;
	}
/*============================================================================*/
/*===== VT_mc34704_SU_cleanup =====*/
/**
@brief  assumes the post-condition of the test case execution

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc34704_SU_cleanup(void) {
		int rv = TFAIL;

		rv = TPASS;

		return rv;
	}

/*============================================================================*/
/*===== VT_mc34704_test_SU =====*/
/**
@brief  MC34704 test scenario SU function

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc34704_test_SU(void) {
		int rv = TPASS, fd;
		int event = EVENT_FLT5;

		fd = open(MC34704_DEVICE, O_RDWR);
		if (fd < 0) {
			pthread_mutex_lock(&mutex);
			tst_resm(TFAIL, "Unable to open %s", MC34704_DEVICE);
			pthread_mutex_unlock(&mutex);
			return TFAIL;
		}

		pthread_mutex_lock(&mutex);
		/* printf("Test subscribe/un-subscribe event = %d\n", event); */
		pthread_mutex_unlock(&mutex);

		if (VT_mc34704_subscribe(fd, event) != TPASS) {
			rv = TFAIL;
		}
		if (VT_mc34704_unsubscribe(fd, event) != TPASS) {
			rv = TFAIL;
		}

		pthread_mutex_lock(&mutex);
		/* printf("Test subscribe/un-subscribe 2 event = %d\n", event); */
		pthread_mutex_unlock(&mutex);
		if (VT_mc34704_subscribe(fd, event) != TPASS) {
			rv = TFAIL;
		}
		if (VT_mc34704_subscribe(fd, event) != TPASS) {
			rv = TFAIL;
		}
		if (VT_mc34704_unsubscribe(fd, event) != TPASS) {
			rv = TFAIL;
		}
		if (VT_mc34704_unsubscribe(fd, event) != TPASS) {
			rv = TFAIL;
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
