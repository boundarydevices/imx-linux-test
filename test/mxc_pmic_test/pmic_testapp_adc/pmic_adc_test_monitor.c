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
 * @file   pmic_adc_test_monitor.c
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
#include "pmic_adc_test_monitor.h"
#include "pmic_adc_test_read.h"

extern int fd_adc;

/*==============================================================================
                                       LOCAL FUNCTIONS
==============================================================================*/

/*============================================================================*/
/*===== VT_pmic_adc_monitor_setup =====*/
/**
@brief  assumes the initial condition of the test case execution

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_adc_monitor_setup(void)
{
    int rv = TFAIL;
    rv=TPASS;
    return rv;
}

/*============================================================================*/
/*===== VT_pmic_adc_monitor_cleanup =====*/
/**
@brief  assumes the post-condition of the test case execution

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_adc_monitor_cleanup(void)
{
    int rv = TFAIL;
    rv=TPASS;
    return rv;
}

/*============================================================================*/
static void callback_wcomp(void)
{
        printf(("\tADC Monitoring test event"));
}

/*============================================================================*/
/*===== VT_pmic_adc_test_monitor =====*/
/**
@brief  PMIC test scenario monitor functionG

@param  None

@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_adc_test_monitor(void)
{
        t_adc_comp_param mon_param;

        mon_param.wlow = 3;
        mon_param.whigh = 60;
        mon_param.channel = BATTERY_CURRENT;
        mon_param.callback = (t_comparator_cb*)callback_wcomp;

        printf(("Test monitoring ADC functions\n"));
        if (ioctl(fd_adc, PMIC_ADC_ACTIVATE_COMPARATOR, & mon_param) != 0) {
                printf(("Error in ADC monitoring test\n"));
                return TFAIL;
        }
        if (ioctl(fd_adc, PMIC_ADC_DEACTIVE_COMPARATOR, 0) != 0) {
                printf(("Error in ADC monitoring test\n"));
                return TFAIL;
        }
        return TPASS;
}

#ifdef __cplusplus
}
#endif
