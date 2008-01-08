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
 * @file   pmic_adc_test_TS.h
 * @brief  Test scenario C header PMIC.
 */

#ifndef PMIC_ADC_TEST_TS_H
#define PMIC_ADC_TEST_TS_H

#ifdef __cplusplus
extern "C"{
#endif

/*==============================================================================
                                         INCLUDE FILES
==============================================================================*/

/*==============================================================================
                                           CONSTANTS
==============================================================================*/

/*==============================================================================
                                       DEFINES AND MACROS
==============================================================================*/

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
int VT_pmic_adc_TS_setup();
int VT_pmic_adc_TS_cleanup();

int VT_pmic_adc_test_TS(void);

#ifdef __cplusplus
}
#endif

#endif  // PMIC_ADC_TEST_TS_H //
