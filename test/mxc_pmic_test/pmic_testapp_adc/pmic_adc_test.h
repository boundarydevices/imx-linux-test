/* 
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved. 
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
 * @file   pmic_adc_test.h
 * @brief  Test scenario C header PMIC.
 */

#ifndef PMIC_ADC_TEST_H
#define PMIC_ADC_TEST_H

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

#include <asm/arch/pmic_external.h>   
#include <asm/arch/pmic_adc.h>

/*==============================================================================
                                           CONSTANTS
==============================================================================*/

/*==============================================================================
                                       DEFINES AND MACROS
==============================================================================*/

#define    TEST_CASE_TS	        "TS"
#define    TEST_CASE_R		"read"
#define    TEST_CASE_CC	        "CC"
#define    TEST_CASE_IT		"IT"
#define    TEST_CASE_MON        "MON"
#define    TEST_CASE_CONV	"CONV"

#define PMIC_DEVICE_ADC	"/dev/pmic_adc"

/*==============================================================================
                                             ENUMS
==============================================================================*/

/*==============================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/

/*==============================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==============================================================================*/

/*==============================================================================
                                     FUNCTION PROTOTYPES
==============================================================================*/
#ifdef __cplusplus
}
#endif

#endif  // PMIC_ADC_TEST_H //
