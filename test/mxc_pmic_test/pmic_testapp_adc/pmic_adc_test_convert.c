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
 * @file   pmic_adc_test_convert.c
 * @brief  Test scenario C source PMIC.
 */

#ifdef __cplusplus
extern "C"{
#endif

/*==============================================================================
                                        INCLUDE FILES
==============================================================================*/
/* Standard Include Files */
#include <errno.h>

/* Harness Specific Include Files. */
#include "test.h"

/* Verification Test Environment Include Files */
#include "pmic_adc_test.h"
#include "pmic_adc_test_convert.h"
#include "pmic_adc_test_read.h"

extern int fd_adc;

/*==============================================================================
                                       LOCAL FUNCTIONS
==============================================================================*/

/*============================================================================*/
/*===== VT_pmic_adc_convert_setup =====*/
/**
@brief  assumes the initial condition of the test case execution

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_adc_convert_setup(void)
{
    int rv = TFAIL;
    rv=TPASS;
    return rv;
}

/*============================================================================*/
/*===== VT_pmic_adc_convert_cleanup =====*/
/**
@brief  assumes the post-condition of the test case execution

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_adc_convert_cleanup(void)
{
    int rv = TFAIL;
    rv=TPASS;
    return rv;
}

/*============================================================================*/
/*===== VT_pmic_adc_test_convert =====*/
/**
@brief  PMIC test scenario convert function

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_adc_test_convert(void)
{
        t_adc_convert_param adc_convert_param;
        int i;
        int option;

        printf("========== TESTING PMIC adc DRIVER ==========\n");
        printf("Select a channel [0-15] : \n");
        printf("=>");
        scanf("%d", &option);

        printf(("Test convert ADC functions\n"));
        adc_convert_param.channel = option;
        if (ioctl(fd_adc, PMIC_ADC_CONVERT, &adc_convert_param) != 0) {
                printf(("Error in ADC convert test\n"));
                return TFAIL;
        }
        printf(("Channel %d: %d\n"),option,adc_convert_param.result[0]);
        if (ioctl(fd_adc, PMIC_ADC_CONVERT_8X, &adc_convert_param) != 0) {
                printf(("Error in ADC convert_8x test\n"));
                return TFAIL;
        }
        for (i=0;i<=7;i++) {
                printf(("Convert 8x channel %d - %d: %d\n"),option,i,
        		adc_convert_param.result[i]);
        }
        if (ioctl(fd_adc, PMIC_ADC_CONVERT_MULTICHANNEL,
        		&adc_convert_param) != 0) {
                printf(("Error in ADC convert_multichannel test\n"));
                return TFAIL;
        }
        for (i=0;i<=7;i++) {
                printf(("MULTICHANNEL From channel %d - %d: %d\n"),option,
        		i,adc_convert_param.result[i]);
        }

        return TPASS;
}

#ifdef __cplusplus
}
#endif
