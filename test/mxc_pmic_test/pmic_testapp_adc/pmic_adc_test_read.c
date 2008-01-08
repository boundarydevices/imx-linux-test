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
 * @file   pmic_adc_test_read.c 
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
/*===== VT_pmic_adc_read_setup =====*/
/**
@brief  assumes the initial condition of the test case execution

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_adc_read_setup(void)
{
    int rv = TFAIL;
    
    rv=TPASS;
    
    return rv;
}

/*============================================================================*/
/*===== VT_pmic_adc_read_cleanup =====*/
/**
@brief  assumes the post-condition of the test case execution

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_adc_read_cleanup(void)
{
    int rv = TFAIL;
    
    rv=TPASS;
    
    return rv;
}

/*============================================================================*/
/*===== VT_pmic_adc_test_read =====*/
/**
@brief  PMIC Digitizer test scenario read function

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_adc_test_read(void)
{
        int rv, trv;
        int new_count=0;
        short *buffer;
        int count=4096;
        short x, y;

        if ((buffer = (short *)malloc(count*sizeof(short))) == NULL)
        {
                printf("Unable to allocate %d bytes\n", count);
                return TFAIL;
        }

        if (ioctl(fd_adc, TOUCH_SCREEN_READ_INSTALL, NULL) != 0)
        {
                perror("ADC Touch Screen Interface ERROR\n");
                free(buffer);
                return TFAIL;
        }
        printf("\nPress a key to read touch screen value\n");
        printf("Press 'E' to exit\n");

        do {
                trv = read(fd_adc, buffer, new_count);
                x = *(buffer+1);
                y = *(buffer+2);
                printf("Values:\t%d:%d:%d:%d\tReturn:%d\n", *(buffer), x, 
        		y, *(buffer+3), trv);
                if (trv < 0)
                {
                        tst_resm(TFAIL, "Not valid coordinates\n");
                        rv  = TFAIL;
                }
        } while (getchar() != 'E');

        printf("Test touch screen uninstall\n");
        if (ioctl(fd_adc, TOUCH_SCREEN_READ_UNINSTALL, NULL) != 0)
        {
                perror("ADC Touch Screen Interface ERROR\n");
                rv  = TFAIL;
        }

        free(buffer);
        return rv;
}

#ifdef __cplusplus
}
#endif
