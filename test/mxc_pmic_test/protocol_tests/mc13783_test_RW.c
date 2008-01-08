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
 * @file   mc13783_test_RW.c 
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
#include "mc13783_test_RW.h"

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

#define nb_reg_tested 2
	unsigned int REG_TEST[nb_reg_tested] = {
		REG_MEMORY_A,
		REG_MEMORY_B,
	};

#define nb_value 4
	unsigned int TEST_VALUE_RW[nb_value] = { 0x000000,
		0xFFFFFF,
		0x555555,
		0xAAAAAA
	};

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
	int VT_mc13783_RW_setup(void) {
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
	int VT_mc13783_RW_cleanup(void) {
		int rv = TFAIL;

		rv = TPASS;

		return rv;
	}

/*============================================================================*/
/*===== VT_mc13783_test_RW =====*/
/**
@brief  MC13783 test scenario Read / Write function

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc13783_test_RW(void) {
		int rv = TPASS, fd, i, j;
		unsigned int valueW = 0, valueR = 0, reg = 0;

		fd = open(MC13783_DEVICE, O_RDWR);
		if (fd < 0) {
			pthread_mutex_lock(&mutex);
			tst_resm(TFAIL, "Unable to open %s", MC13783_DEVICE);
			pthread_mutex_unlock(&mutex);
			return TFAIL;
		}

		for (i = 0; i < nb_reg_tested; i++) {
			reg = REG_TEST[i];
			for (j = 0; j < nb_value; j++) {
				valueW = TEST_VALUE_RW[j];
				valueR = 0;

				if (VT_mc13783_write(fd, reg, valueW) != TPASS) {
					rv = TFAIL;
				} else {
					if (VT_mc13783_read(fd, reg, &valueR) !=
					    TPASS) {
						rv = TFAIL;
					}
					if (valueR != valueW) {
						pthread_mutex_lock(&mutex);
						printf
						    ("Test ERROR Read/Write res1=0x%X"
						     " val=0x%X\n", valueW,
						     valueR);
						pthread_mutex_unlock(&mutex);
						rv = TFAIL;
					}
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
