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
 * @file   pmic_adc_test_TS.c
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
#include "pmic_adc_test_TS.h"
#include "pmic_adc_test_read.h"

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

extern int fd_adc;

/*==============================================================================
                                   LOCAL FUNCTION PROTOTYPES
==============================================================================*/

/*==============================================================================
                                       LOCAL FUNCTIONS
==============================================================================*/

/*============================================================================*/
/*===== VT_pmic_adc_TS_setup =====*/
/**
@brief  assumes the initial condition of the test case execution

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_adc_TS_setup(void)
{
    int rv = TFAIL;
    
    rv=TPASS;
    
    return rv;
}

/*============================================================================*/
/*===== VT_pmic_adc_TS_cleanup =====*/
/**
@brief  assumes the post-condition of the test case execution

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_adc_TS_cleanup(void)
{
    int rv = TFAIL;
    
    rv=TPASS;
    
    return rv;
}

/*============================================================================*/
/*===== VT_pmic_adc_test_TS =====*/
/**
@brief  PMIC test scenario TS function

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_adc_test_TS(void)
{
        t_touch_screen ts_value;

        printf(("Test Touch Screen ADC\n"));
        if (ioctl(fd_adc, PMIC_ADC_GET_TOUCH_SAMPLE, &ts_value) != 0) {
                printf(("Error in ADC test\n"));
                return TFAIL;
        }
        printf(("Value of X position is %d\n"), (int) ts_value.x_position);
        printf(("Value of Y position is %d\n"), (int) ts_value.y_position);
        printf(("Value of contact resistance is %d\n"), 
                (int) ts_value.contact_resistance);
        
        if ((ts_value.x_position < 0) | (ts_value.x_position > 1000) |
                (ts_value.y_position < 0) | (ts_value.y_position > 1000))
        {
                tst_resm(TFAIL, "Not valid coordinates\n");
                return TFAIL;
        } else
                return TPASS;
}

#ifdef __cplusplus
}
#endif
