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
 * @file   pmic_adc_test_convert.h
 * @brief  Test scenario C header PMIC.
 */

#ifndef PMIC_ADC_TEST_CONVERT_H
#define PMIC_ADC_TEST_CONVERT_H

#ifdef __cplusplus
extern "C"{
#endif

/*==============================================================================
                                     FUNCTION PROTOTYPES
==============================================================================*/
int VT_pmic_adc_convert_setup();
int VT_pmic_adc_convert_cleanup();
int VT_pmic_adc_test_convert(void);

#ifdef __cplusplus
}
#endif

#endif  // PMIC_ADC_TEST_CONVERT_H //
