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
 * @file   mc13783_test_RA.c 
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
#include <time.h>

/* Harness Specific Include Files. */
#include "../include/test.h"

/* Verification Test Environment Include Files */
#include "mc13783_test_common.h"
#include "mc13783_test_RA.h"

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
	unsigned int TEST_VALUE_RA[2 * nb_value][3];
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
	int VT_mc13783_RA_setup(void) {
		int rv = TFAIL;

		 rv = TPASS;

		 return rv;
	}
/*============================================================================*//*===== VT_cleanup =====*//**
@brief  assumes the post-condition of the test case execution

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*//*============================================================================*/ int VT_mc13783_RA_cleanup(void) {
		int rv = TFAIL;

		rv = TPASS;

		return rv;
	}

/*============================================================================*/
/*===== VT_mc13783_test_RA =====*/
/**
@brief  MC13783 test scenario Random Access

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc13783_test_RA(void) {
		int rv = TPASS, fd, i = 0, val;
		srand((unsigned)time(NULL));

		fd = open(MC13783_DEVICE, O_RDWR);
		if (fd < 0) {
			pthread_mutex_lock(&mutex);
			tst_resm(TFAIL, "Unable to open %s", MC13783_DEVICE);
			pthread_mutex_unlock(&mutex);
			return TFAIL;
		}

		for (i = 0; i < nb_value; i++) {
			val = rand() % 2;
			switch (val) {
			case 0:
				TEST_VALUE_RA[i][0] = CMD_WRITE;
				TEST_VALUE_RA[i][1] = rand() % REG_NB;
				TEST_VALUE_RA[i][2] = rand() % 0xFFFFFF;
				TEST_VALUE_RA[nb_value + i][0] = CMD_READ;
				TEST_VALUE_RA[nb_value + i][1] =
				    rand() % REG_NB;
				TEST_VALUE_RA[nb_value + i][2] =
				    rand() % 0xFFFFFF;
				break;
			case 1:
				TEST_VALUE_RA[i][0] = CMD_SUB;
				TEST_VALUE_RA[i][1] = rand() % EVENT_NB;
				TEST_VALUE_RA[i][2] = 0;
				TEST_VALUE_RA[nb_value + i][0] = CMD_UNSUB;
				TEST_VALUE_RA[nb_value + i][1] =
				    rand() % EVENT_NB;
				TEST_VALUE_RA[nb_value + i][2] = 0;
				break;
			}
		}

		for (i = 0; i < 2 * nb_value; i++) {
			if (VT_mc13783_opt
			    (fd, TEST_VALUE_RA[i][0], TEST_VALUE_RA[i][1],
			     &(TEST_VALUE_RA[i][2])) != TPASS) {
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
