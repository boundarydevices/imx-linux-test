/*
 * Copyright 2008-2009 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file   mc34704_test_common.h
 * @brief  Test scenario C header PMIC.
 */

#ifndef MC34704_TEST_COMMON_H
#define MC34704_TEST_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
                                         INCLUDE FILES
==============================================================================*/

#include <sys/types.h>		/* open() */
#include <sys/stat.h>		/* open() */
#include <fcntl.h>		/* open() */
#include <sys/ioctl.h>		/* ioctl() */
#include <unistd.h>		/* close() */
#include <stdio.h>		/* sscanf() & perror() */
#include <stdlib.h>		/* atoi() */
#include <pthread.h>

#include <linux/pmic_external.h>

/*==============================================================================
                                           CONSTANTS
==============================================================================*/

/*==============================================================================
                                       DEFINES AND MACROS
==============================================================================*/

#define    TEST_CASE_RW                "RW"
#define    TEST_CASE_SU                "SU"
#define    TEST_CASE_S_IT_U            "S_IT_U"
#define    TEST_CASE_D                 "D"
#define    TEST_CASE_OC                "OC"
#define    TEST_CASE_CA                "CA"
#define    TEST_CASE_RA                "RA"
#define    TEST_CASE_IP                "IP"

#define    CMD_READ         0
#define    CMD_WRITE        1
#define    CMD_SUB          2
#define    CMD_UNSUB        3

#if !defined(TRUE) && !defined(FALSE)
#define TRUE         1
#define FALSE        0
#endif

#define MC34704_DEVICE           "/dev/pmic"
#define MAX_REG                50

	typedef struct {
		// operation - performing operation (CMD_READ, CMD_WRITE, CMD_SUB,
		//             CMD_UNSUB)
		int operation;
		// val1 - reg number if operation is CMD_READ or CMD_WRITE,
		// event number if operation is CMD_SUB or CMD_UNSUB,
		int val1;
		// *val2 - writing reg value in case of CMD_WRITE operation
		// *val2 - return reg value in case of CMD_READ operation
		// not used in CMD_SUB or CMD_UNSUB operations
		unsigned int *val2;
	} opt_params;

/*==============================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==============================================================================*/

extern	int verbose_flag;
extern	pthread_mutex_t mutex;

/*==============================================================================
                                     FUNCTION PROTOTYPES
==============================================================================*/
	int VT_mc34704_setup();
	int VT_mc34704_cleanup();

	int VT_mc34704_read(int device, int reg, unsigned int *val);
	int VT_mc34704_write(int device, int reg, unsigned int val);
	int VT_mc34704_subscribe(int device, unsigned int event);
	int VT_mc34704_unsubscribe(int device, unsigned int event);

	int VT_mc34704_opt(int fd, int operation, int val1, unsigned int *val2);

#ifdef __cplusplus
}
#endif
#endif				// MC34704_TEST_H //
