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
 * @file   mc13783_test_S_IT_U.c 
 * @brief  Test scenario C source PMIC.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
                                        INCLUDE FILES
==============================================================================*/
/* Standard Include Files */
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

/* Harness Specific Include Files. */
#include "../include/test.h"

/* Verification Test Environment Include Files */
#include "mc13783_test_common.h"
#include "mc13783_test_S_IT_U.h"

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
/*===== VT_mc13783_S_IT_U_setup =====*/
/**
@brief  assumes the initial condition of the test case execution

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc13783_S_IT_U_setup(void) {
		int rv = TFAIL;

		 rv = TPASS;

		 return rv;
	}
															 /*============================================================================*//*===== VT_mc13783_S_IT_U_cleanup =====*//**
@brief  assumes the post-condition of the test case execution

@param  None
  
@return On success - return TPASS
          On failure - return the error code
*//*============================================================================*/ int VT_mc13783_S_IT_U_cleanup(void) {
		int rv = TFAIL;

		rv = TPASS;

		return rv;
	}

	int ask_user(char *question) {
		unsigned char answer;
		int ret = TRETR;

		printf("%s [Y/N]", question);
		do {
			answer = fgetc(stdin);
			if (answer == 'Y' || answer == 'y')
				ret = TPASS;
			else if (answer == 'N' || answer == 'n')
				ret = TFAIL;
		} while (ret == TRETR);
		fgetc(stdin);	/* Wipe CR character from stream */
		return ret;
	}

/*============================================================================*/
/*===== VT_mc13783_test_S_IT_U =====*/
/**
@brief  MC13783 test scenario IT function

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
	int VT_mc13783_test_S_IT_U(void) {
		int rv = TPASS, fd;
		char result;
		int event = EVENT_ONOFD1I;

		fd = open(MC13783_DEVICE, O_RDWR);
		if (fd < 0) {
			pthread_mutex_lock(&mutex);
			tst_resm(TFAIL, "Unable to open %s", MC13783_DEVICE);
			pthread_mutex_unlock(&mutex);
			return TFAIL;
		}

		if (VT_mc13783_subscribe(fd, event) != TPASS) {
			rv = TFAIL;
		}

		pthread_mutex_lock(&mutex);
		printf("\nPress PWR button on the KeyBoard or MC13783\n"
		       "you should see IT callback info\n"
		       "Press Enter to continue after pressing the button\n");
		getchar();
		pthread_mutex_unlock(&mutex);
		if (VT_mc13783_unsubscribe(fd, event) != TPASS) {
			rv = TFAIL;
		}

		pthread_mutex_lock(&mutex);
		if (ask_user("Did you see the callback info ") == TFAIL) {
			rv = TFAIL;
		}
		printf("Test subscribe/unsubscribe 2 event = %d\n", event);
		pthread_mutex_unlock(&mutex);
		if (VT_mc13783_subscribe(fd, event) != TPASS) {
			rv = TFAIL;
		}
		if (VT_mc13783_subscribe(fd, event) != TPASS) {
			rv = TFAIL;
		}

		pthread_mutex_lock(&mutex);
		printf("\nPress PWR button on the KeyBoard or MC13783\n"
		       "you should see IT callback info twice.\n"
		       "Press Enter to continue after pressing the button\n");
		getchar();
		pthread_mutex_unlock(&mutex);
		if (VT_mc13783_unsubscribe(fd, event) != TPASS) {
			rv = TFAIL;
		}
		if (VT_mc13783_unsubscribe(fd, event) != TPASS) {
			rv = TFAIL;
		}
		if (ask_user("Did you see the callback info twice") == TFAIL) {
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
