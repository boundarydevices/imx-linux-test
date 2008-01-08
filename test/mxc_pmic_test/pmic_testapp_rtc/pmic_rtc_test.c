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
 * @file   pmic_rtc_test.c 
 * @brief  Test scenario C source PMIC.
 */

#ifdef __cplusplus
extern "C"{
#endif

/*==============================================================================
                                        INCLUDE FILES
==============================================================================*/
    
/* Harness Specific Include Files. */

#include "test.h"
#include <time.h>
#include <poll.h>

/* Verification Test Environment Include Files */
#include "pmic_rtc_test.h"

/*==============================================================================
                                        LOCAL MACROS
==============================================================================*/

/*==============================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==============================================================================*/

/*==============================================================================
                                       LOCAL CONSTANTS
==============================================================================*/

char *jours[]= {"Sunday", 
                "monday", 
                "tuesday", 
                "Wednesday", 
                "Thursday", 
                "Friday", 
                "Saturday"};

char *mois[]= {"January", 
               "February", 
               "March", 
               "April", 
               "May", 
               "June",
               "July", 
               "August", 
               "September", 
               "October", 
               "November", 
               "December"};

/*==============================================================================
                                       LOCAL VARIABLES
==============================================================================*/

/* struct semaphore  timer_sema; */

/*==============================================================================
                                       GLOBAL CONSTANTS
==============================================================================*/

/*==============================================================================
                                       GLOBAL VARIABLES
==============================================================================*/

/*==============================================================================
                                   LOCAL FUNCTION PROTOTYPES
==============================================================================*/

/*==============================================================================
                                       LOCAL FUNCTIONS
==============================================================================*/

/*============================================================================*/
/*===== VT_PMIC_TEST_rtc_setup =====*/
/**
@brief  assumes the initial condition of the test case execution

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_rtc_test_setup(void)
{
    int rv = TFAIL;
    
    rv=TPASS;
    
    return rv;
}

/*============================================================================*/
/*===== VT_PMIC_TEST_rtc_cleanup =====*/
/**
@brief  assumes the post-condition of the test case execution

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
void VT_pmic_rtc_test_cleanup(void)
{
    tst_exit();
}

/*============================================================================*/
/*===== VT_PMIC_test_rtc_TEST =====*/
/**
@brief  PMIC test scenario X function

@param  None
  
@return On success - return TPASS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_rtc_test(int switch_fct)
{
        int fd, ret;
        struct timeval pmic_time, pmic_time_read;
        time_t maintenant;
        struct tm *m;
        int VT_rv = TFAIL;
        struct pollfd fds;
        
        fd = open(PMIC_DEVICE_RTC, O_RDWR);
        if (fd < 0) {
                tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                        "Unable to open %s\n", VT_rv);
        }
        
        if (switch_fct == 0) {
                tst_resm(TINFO, "Get PMIC time\n");
                ret = ioctl(fd, PMIC_RTC_GET_TIME, &pmic_time_read);
                if (ret != 0) {
                        tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                                 "Error in rtc test\n", VT_rv);
                }
                
                m= localtime(&pmic_time_read.tv_sec);
                
                tst_resm(TINFO, "%ld seconds between the current time\n\t\t\t"
                        "and midnight, January 1, 1970 UTC", 
                        pmic_time_read.tv_sec);
                tst_resm(TINFO, "The PMIC time is : %02d/%02d/%02d %02d:%02d:"
                        "%02d",m->tm_mon + 1,m->tm_mday, m->tm_year % 100, 
                        m->tm_hour, m->tm_min, m->tm_sec);
                tst_resm(TINFO, "%s, %s %d, %04d %de day of the year", 
                        jours[m->tm_wday], mois[m->tm_mon], m->tm_mday, 
                        m->tm_year + 1900, m->tm_yday);        
               
        }
        
        if (switch_fct == 1) {
                time(&maintenant);
                pmic_time.tv_sec = maintenant;
                
                tst_resm(TINFO, "%ld seconds between the current time and "
                        "midnight, January 1, 1970 UTC\n", maintenant);
                
                m= localtime(&maintenant);
                
                tst_resm(TINFO, "The date and time is : %02d/%02d/%02d %02d:"
                               "%02d:%02d\n",m->tm_mon + 1,m->tm_mday, 
                        m->tm_year % 100, m->tm_hour, m->tm_min, m->tm_sec);
                tst_resm(TINFO, "%s, %s %d, %04d %de day of the year\n", 
                        jours[m->tm_wday], mois[m->tm_mon], m->tm_mday, 
                        m->tm_year + 1900, m->tm_yday);        
                
                tst_resm(TINFO, "Set PMIC time\n");
                ret = ioctl(fd, PMIC_RTC_SET_TIME, &pmic_time);
                if (ret != 0) {
                        tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                        "Error in rtc test\n", VT_rv);
                        
                }
                
                tst_resm(TINFO, "Get PMIC time\n");
                ret = ioctl(fd, PMIC_RTC_GET_TIME, &pmic_time_read);
                if (ret != 0) {
                        tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                                 "Error in rtc test\n", VT_rv);
                }
                
                tst_resm(TINFO, "Return value of PMIC time is %ld\n",
                        pmic_time_read.tv_sec);
                
                if (pmic_time.tv_sec > pmic_time_read.tv_sec) {
                        if ((pmic_time.tv_sec+5) < pmic_time_read.tv_sec) {
                                tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                                        "Error in RTC test\n", VT_rv);
                        }
                }      
        }      
        if (switch_fct == 2) {
                time(&maintenant);
                pmic_time.tv_sec = maintenant;
                
                tst_resm(TINFO, "%ld seconds between the current time and "
                        "midnight, January 1, 1970 UTC\n", maintenant);
                
                m= localtime(&maintenant);
                
                tst_resm(TINFO, "The date and time is : %02d/%02d/%02d %02d:"
                        "%02d:%02d\n",m->tm_mon + 1,m->tm_mday, 
                        m->tm_year % 100, m->tm_hour, m->tm_min, m->tm_sec);
                tst_resm(TINFO, "%s, %s %d, %04d %de day of the year\n", 
                        jours[m->tm_wday], mois[m->tm_mon], m->tm_mday, 
                        m->tm_year + 1900, m->tm_yday);        
                
                tst_resm(TINFO, "Set PMIC Alarm time\n");
                ret = ioctl(fd, PMIC_RTC_SET_ALARM, &pmic_time);
                if (ret != 0) {
                        tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                                "Error in rtc test\n", VT_rv);
                }
                
                tst_resm(TINFO, "Get PMIC Alarm time\n");
                ret = ioctl(fd, PMIC_RTC_GET_ALARM, &pmic_time_read);
                if (ret != 0) {
                        tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                                "Error in rtc test\n", VT_rv);
                }
                
                tst_resm(TINFO, "Return value of PMIC Alarm time is %ld\n",
                        pmic_time_read.tv_sec);
                
                if (pmic_time.tv_sec != pmic_time_read.tv_sec) {
                        tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                                "Error in alarm rtc test\n", VT_rv);
                }      
        }
        
        if (switch_fct == 3) {
                time(&maintenant);
                pmic_time.tv_sec = maintenant;
                
                tst_resm(TINFO, "Test PMIC Alarm event\n");
                
                
                tst_resm(TINFO, "Set PMIC time to local time\n");
                ret = ioctl(fd, PMIC_RTC_SET_TIME, &pmic_time);
                if (ret != 0) {
                        tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                                "Error in rtc test\n", VT_rv);
                }
                
                tst_resm(TINFO, 
                        "Set PMIC Alarm time to local time +10 second\n");
                pmic_time.tv_sec = pmic_time.tv_sec + 10;
                ret = ioctl(fd, PMIC_RTC_SET_ALARM, &pmic_time);
                if (ret != 0) {
                        tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                                "Error in rtc test\n", VT_rv);
                }
                
                tst_resm(TINFO, "Wait Alarm event...\n");
                ret = ioctl(fd, PMIC_RTC_WAIT_ALARM, NULL);
                if (ret != 0) {
                        tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                                "Error in IT rtc test\n", VT_rv);
                }
                
                tst_resm(TINFO, "*** Alarm event DONE ***\n");
                
        }
        if (switch_fct == 4) {
                time(&maintenant);
                pmic_time.tv_sec = maintenant;
                
                tst_resm(TINFO, "Test PMIC Alarm event\n");
                
                tst_resm(TINFO, "Set PMIC time to local time\n");
                ret = ioctl(fd, PMIC_RTC_SET_TIME, &pmic_time);
                if (ret != 0) {
                        tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                                "Error in rtc test\n", VT_rv);
                }
                
                tst_resm(TINFO, 
                        "Set PMIC Alarm time to local time +10 second\n");
                pmic_time.tv_sec = pmic_time.tv_sec + 10;
                ret = ioctl(fd, PMIC_RTC_SET_ALARM, &pmic_time);
                if (ret != 0) {
                        tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                                "Error in rtc test\n", VT_rv);
                }
                
                fds.fd = fd;
                fds.events = POLLIN;
                fds.revents = 0;

                tst_resm(TINFO, "Call poll function before register\n");
                ret = poll(&fds,1,0);
                tst_resm(TINFO, "Poll ret = %d\n",ret);

                tst_resm(TINFO, "Register Alarm\n");
                ret = ioctl(fd, PMIC_RTC_ALARM_REGISTER,0);
                if (ret != 0) {
                        tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                                "Error in IT rtc test\n", VT_rv);
                }
                
                tst_resm(TINFO, "Call poll until alarm\n");
                
                do {
                        ret = poll(&fds,1,0);
                } while (ret == 0);

                
                tst_resm(TINFO, "Unregister Alarm\n");
                ret = ioctl(fd, PMIC_RTC_ALARM_UNREGISTER,0);
                if (ret != 0) {
                        tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                                "Error in IT rtc test\n", VT_rv);
                }
                
        }
        
        ret = close(fd);
        if (ret < 0) {
                tst_resm(TINFO, "Unable to close file descriptor %d", fd);
                tst_brkm(TBROK , VT_pmic_rtc_test_cleanup, 
                        "Error in rtc test\n", VT_rv);
        }
        return TPASS;
}

#ifdef __cplusplus
}
#endif
