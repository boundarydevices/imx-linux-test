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
 * @file   mc13783_test_common.c 
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
/*===== VT_mc13783_setup =====*/
/**
@brief  assumes the initial condition of the test case execution

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc13783_setup(void) {
		int rv = TFAIL;
		 rv = pthread_mutex_init(&mutex, NULL);

		 return rv;
	}
/*============================================================================*//*===== VT_cleanup =====*//**
@brief  assumes the post-condition of the test case execution

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*//*============================================================================*/ int VT_mc13783_cleanup(void) {
		int rv = TFAIL;

		rv = pthread_mutex_destroy(&mutex);

		return rv;
	}

/*============================================================================*/
/*===== VT_mc13783_read =====*/
/**
@brief  read register

@param  Input :                fd - file descriptor assigned to the mc13783
                               reg - reg number
                               *val - return read value
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/

	int VT_mc13783_read(int fd, int reg, unsigned int *val) {
		register_info reg_info;
		reg_info.reg = reg;
		reg_info.reg_value = 0;

		int rv = ioctl(fd, PMIC_READ_REG, &reg_info);
		*val = reg_info.reg_value;

		if (rv != 0) {
			if (verbose_flag) {

				pthread_mutex_lock(&mutex);
				printf("Read value from reg %d: FAILED\n", reg);
				pthread_mutex_unlock(&mutex);
			}
			return TFAIL;
		}
		if (verbose_flag) {
			pthread_mutex_lock(&mutex);
			printf("Read value from reg %d: 0x%X\n", reg, *val);
			pthread_mutex_unlock(&mutex);
		}
		return TPASS;
	}

/*============================================================================*/
/*===== VT_mc13783_write =====*/
/**
@brief  writes register

@param  Input :                fd - file descriptor assigned to the mc13783
                               reg - reg number
                               *val - writing value

@return On success - return TPASS
        On failure - return TFAIL

*/
/*============================================================================*/

	int VT_mc13783_write(int fd, int reg, unsigned int val) {
		register_info reg_info;
		reg_info.reg = reg;
		reg_info.reg_value = val;

		int rv = ioctl(fd, PMIC_WRITE_REG, &reg_info);

		if (rv != 0) {
			if (verbose_flag) {
				pthread_mutex_lock(&mutex);
				printf("Write value 0x%X in reg %d: FAILED\n",
				       val, reg);
				pthread_mutex_unlock(&mutex);
			}
			return TFAIL;
		}
		if (verbose_flag) {
			pthread_mutex_lock(&mutex);
			printf("Write value 0x%X in reg %d: OK\n", val, reg);
			pthread_mutex_unlock(&mutex);
		}
		return TPASS;
	}

/*============================================================================*/
/*===== VT_mc13783_subscribe =====*/
/**
@brief  Subscribe event

@param  Input :                fd - file descriptor assigned to the mc13783
                               event - event number

@return On success - return TPASS
        On failure - return TFAIL

*/
/*============================================================================*/

	int VT_mc13783_subscribe(int fd, unsigned int event) {

		int rv = ioctl(fd, PMIC_SUBSCRIBE, &event);
		if (rv != 0) {
			if (verbose_flag) {
				pthread_mutex_lock(&mutex);
				printf("Subscribe event %d: FAILED\n", event);
				pthread_mutex_unlock(&mutex);
			}
			return TFAIL;
		}
		if (verbose_flag) {
			pthread_mutex_lock(&mutex);
			printf("Subscribe event %d: OK\n", event);
			pthread_mutex_unlock(&mutex);
		}
		return TPASS;
	}

/*============================================================================*/
/*===== VT_mc13783_unsubscribe =====*/
/**
@brief  Unsubscribe event

@param  Input :                fd - file descriptor assigned to the mc13783
                               event - event number

@return On success - return TPASS
        On failure - return TFAIL
*/
/*============================================================================*/

	int VT_mc13783_unsubscribe(int fd, unsigned int event) {

		int rv = ioctl(fd, PMIC_UNSUBSCRIBE, &event);
		if (rv != 0) {
			if (verbose_flag) {
				pthread_mutex_lock(&mutex);
				printf("Unsubscribe event %d: FAILED\n", event);
				pthread_mutex_unlock(&mutex);
			}
			return TFAIL;
		}
		if (verbose_flag) {
			pthread_mutex_lock(&mutex);
			printf("Unsubscribe event %d: OK\n", event);
			pthread_mutex_unlock(&mutex);
		}
		return TPASS;
	}

/*============================================================================*/
/*===== VT_mc13783_opt =====*/
/**
@brief  perform operations (read reg, write reg, subscribe event, Unsubscribe 
        event) on already opened MC13783 file

@param  Input : fd - file descriptor assigned to the mc13783
                operation - performing operation (CMD_READ, CMD_WRITE, CMD_SUB,
        	 	    CMD_UNSUB)
                val1 - reg number if operation is CMD_READ or CMD_WRITE,
                       event number in case of CMD_SUB or CMD_UNSUB operations
                *val2 - writing reg value in case of CMD_WRITE operation
        Output: *val2 - return reg value in case of CMD_READ operation

@return On success - return TPASS
        On failure - return TFAIL

*/
/*============================================================================*/

	int VT_mc13783_opt(int fd, int operation, int val1, unsigned int *val2) {
		int rv;
		switch (operation) {
		case CMD_READ:
			rv = VT_mc13783_read(fd, val1, val2);
			break;
		case CMD_WRITE:
			rv = VT_mc13783_write(fd, val1, *val2);
			break;
		case CMD_SUB:
			rv = VT_mc13783_subscribe(fd, val1);
			break;
		case CMD_UNSUB:
			rv = VT_mc13783_unsubscribe(fd, val1);
			break;

		default:
			tst_resm(TFAIL, "Unsupported operation");
			rv = TFAIL;
			break;
		}
		return rv;
	}

#ifdef __cplusplus
}
#endif
