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
 * @file   mc34704_test_IP.c
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
#include "mc34704_test_IP.h"

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

#define nb_REG_RO 4
	unsigned int TEST_REG_RO[nb_REG_RO] = { REG_MC34704_GENERAL3,
		REG_MC34704_FAULTS,
		REG_MC34704_REG4SET2,
		REG_MC34704_VGSET2,
	};

#define nb_REG_NA 3
	unsigned int TEST_REG_NA[nb_REG_NA] = { 0x00,
		0x20,
		0x60,
	};

#define nb_EVNT_NE 1
	unsigned int TEST_EVNT_NA[nb_EVNT_NE] = { EVENT_NB };

	int TEST_VALUE = 0xFFF000;

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
	int VT_mc34704_IP_setup(void) {
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
	int VT_mc34704_IP_cleanup(void) {
		int rv = TFAIL;

		rv = TPASS;

		return rv;
	}

/*============================================================================*/
/*===== VT_mc34704_test_IP =====*/
/**
@brief  MC34704 test scenario Access with incorrect parameters

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc34704_test_IP(void) {
		int rv = TPASS, fd, i = 0;

		fd = open(MC34704_DEVICE, O_RDWR);
		if (fd < 0) {
			pthread_mutex_lock(&mutex);
			tst_resm(TFAIL, "Unable to open %s", MC34704_DEVICE);
			pthread_mutex_unlock(&mutex);
			return TFAIL;
		}
		// writing into Read Only register
		for (i = 0; i < nb_REG_RO; i++) {
			if (VT_mc34704_opt
			    (fd, CMD_WRITE, TEST_REG_RO[i],
			     &TEST_VALUE) == TPASS) {
				pthread_mutex_lock(&mutex);
				printf
				    ("Driver have wrote to Read Only reg %d\n",
				     TEST_REG_RO[i]);
				pthread_mutex_unlock(&mutex);
				rv = TFAIL;
			}

		}

		// writing into non avaible register
		for (i = 0; i < nb_REG_NA; i++) {
			if (VT_mc34704_opt
			    (fd, CMD_WRITE, TEST_REG_NA[i],
			     &TEST_VALUE) == TPASS) {
				pthread_mutex_lock(&mutex);
				printf
				    ("Driver have wrote to non avaible reg %d\n",
				     TEST_REG_NA[i]);
				pthread_mutex_unlock(&mutex);
				rv = TFAIL;
			}
		}

		// reading from non avaible register
		for (i = 0; i < nb_REG_NA; i++) {
			if (VT_mc34704_opt
			    (fd, CMD_READ, TEST_REG_NA[i],
			     &TEST_VALUE) == TPASS) {
				pthread_mutex_lock(&mutex);
				printf
				    ("Driver have read from non avaible reg %d\n",
				     TEST_REG_NA[i]);
				pthread_mutex_unlock(&mutex);
				rv = TFAIL;
			}
		}

		// subscribing non existing event
		for (i = 0; i < nb_EVNT_NE; i++) {
			if (VT_mc34704_opt
			    (fd, CMD_SUB, TEST_EVNT_NA[i],
			     &TEST_VALUE) == TPASS) {
				pthread_mutex_lock(&mutex);
				printf
				    ("Driver have subscribed non existing event %d\n",
				     TEST_EVNT_NA[i]);
				pthread_mutex_unlock(&mutex);
				rv = TFAIL;
			}
		}

		// un-subscribing not subscribed event
		if (VT_mc34704_opt(fd, CMD_UNSUB, 2, &TEST_VALUE) == TPASS) {
			pthread_mutex_lock(&mutex);
			printf
			    ("Driver have unsubscribed not subscribed event %d\n",
			     TEST_EVNT_NA[i]);
			pthread_mutex_unlock(&mutex);
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
