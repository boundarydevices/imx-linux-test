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
 * @file   pmic_battery_test.h
 * @brief  Test scenario C header PMIC.
 */

#ifndef PMIC_TEST_BATT_H
#define PMIC_TEST_BATT_H

#ifdef __cplusplus
extern "C"{
#endif

/*==============================================================================
                                         INCLUDE FILES
==============================================================================*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/wait.h>

#include <linux/pmic_status.h>
#include <linux/pmic_battery.h>

/*==============================================================================
                                       DEFINES AND MACROS
==============================================================================*/
#define	PMIC_BATT_DEV "pmic_battery"

#if !defined(TRUE) && !defined(FALSE)
#define TRUE  1
#define FALSE 0
#endif

/*==============================================================================
                                     FUNCTION PROTOTYPES
==============================================================================*/
int VT_pmic_batt_test(int switch_fct);

#ifdef __cplusplus
}
#endif

#endif
