/*
 * Copyright 2005-2009 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file   pmic_rtc_test.h
 * @brief  Test scenario C header PMIC.
 */

#ifndef PMIC_TEST_RTC_H
#define PMIC_TEST_RTC_H

#ifdef __cplusplus
extern "C"{
#endif

/*==============================================================================
                                         INCLUDE FILES
==============================================================================*/

#include <sys/types.h>	/* open() */
#include <sys/stat.h>	/* open() */
#include <fcntl.h>	/* open() */
#include <sys/ioctl.h>	/* ioctl() */
#include <unistd.h>	/* close() */
#include <stdio.h>	/* sscanf() & perror() */
#include <stdlib.h>	/* atoi() */
#include <errno.h>
#include <linux/wait.h>

#include <linux/pmic_rtc.h>

/*==============================================================================
                                       DEFINES AND MACROS
==============================================================================*/

#define    TEST_CASE1	"TEST"
#define    TEST_CASE2	"TIME"
#define    TEST_CASE3	"ALARM"
#define    TEST_CASE4	"WAIT_ALARM"
#define    TEST_CASE5	"POLL_TEST"

#define PMIC_DEVICE_RTC	"/dev/pmic_rtc"

/*==============================================================================
                                     FUNCTION PROTOTYPES
==============================================================================*/
int VT_pmic_rtc_test_setup();
void VT_pmic_rtc_test_cleanup();
int VT_pmic_rtc_test(int switch_fct);

#ifdef __cplusplus
}
#endif

#endif  // PMIC_TEST_RTC_H //
