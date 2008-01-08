/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <limits.h>
#include "mxc_test.h"
#include <asm/arch/mxc_pm.h>




#define MAX_OP 5
static int op_points[MAX_OP]={ 532, 399, 266, 133, 0 };

static void mxc_set_platform_op(int plat)
{
	
	switch(plat)
		{
	case mx31 :
	op_points[0] = 532;
	op_points[1] = 532;
	op_points[2] = 266;
	op_points[3] = 133;
	op_points[4] = 0;
	
	break;
	case mx27 :
	op_points[0] = 266;
	op_points[1] = 266;
	op_points[2] = 133;
	op_points[3] = 133;
	op_points[4] = 0;
	break;
	case mxc91311 :
	op_points[0] = 399;
	op_points[1] = 266;
	op_points[2] = 0;
	break;
	case mxc92323 :
	op_points[0] = 399;
	op_points[1] = 332;
	op_points[2] = 266;
	op_points[3] = 199;
	op_points[4] = 133;
	op_points[4] = 0;
	
	break;
	default:
	op_points[0] = 532;
	op_points[1] = 399;
	op_points[2] = 266;
	op_points[3] = 133;
	op_points[4] = 0;
	break;
		}
}
int main(int argc, char **argv)
{
        mxc_pm_test pm_test;
        unsigned int fp, option;
	 unsigned long platform[1]={0};
	

       int op_pt_ctr, random_op_pt, seed;
        int point3, point2, point1, point0, new_op_point;
        int pctr0, pctr1, pctr2, pctr3, i;

        printf("========== TESTING Low-level PM DRIVER ==========\n");
        printf("Note that some option #'s may be skipped depending on platform.\n");
        printf("So, enter the option number corresponding to each option correctly\n");
        printf("Enter any of the following options: \n");
        printf("1. Test Integer Scaling \n");
        printf("2. Test PLL Scaling \n");
        printf("3. Test Int/PLL Scaling (choice decided by PM driver) \n");
        printf("Please note that option 3 is not available for MXC91331 \n");
        printf("4. Test Low Power Modes \n");
        printf("5. Select CKOH output \n");
        printf("6. Configure PMIC for High Voltage (Regs at 1.6V) \n");
        printf("7. Configure PMIC for Low Voltage (Regs at 1.2V) \n");
        printf("8. Configure PMIC for High & Low Voltage (Regs at 1.2 & 1.6V) \n");
        printf("Please note that options 6,7,8 are not available for MXC91331 \n");
        printf("9. Infinite loop cycling through operating points. \n");
        printf("Please note that option 9 is not available for MXC91331 \n");

        scanf("%d", &option);
        fp = open("/dev/mxc_test", O_RDWR);
        if (fp < 0) {
                printf("Test module: Open failed with error %d\n", fp);
                perror("Open");
                return -1;
        }
	ioctl(fp, MXCTEST_GET_PLATFORM, &platform);
	//printf("Platform is after ioctl %d \n", *platform);
	mxc_set_platform_op(*platform);
        switch(option) {
        case 1:
                printf("This option is not supported on MXC27530\n");
                printf("Use option 3 to test DVFS\n");
                printf("Enter a valid ARM frequency: \n");
                scanf("%d", &pm_test.armfreq);
                /*
                 * If someone enters just the MHz value, conver to Hz and
                 * assume they want ahb and ipg to be 133, 66.5 MHz.
                 */
                if ( pm_test.armfreq < 1000000 ) {
                        pm_test.armfreq *= 1000000;
		  		if((*platform==mxc91131) || (*platform==mx31))
						{
                        			pm_test.ahbfreq = 133000000;
                        			pm_test.ipfreq =  66500000;
						}
                } else {
                        printf("Enter a valid AHB frequency: \n");
                        scanf("%d", &pm_test.ahbfreq);
                        printf("Enter a valid IPG frequency: \n");
                        scanf("%d", &pm_test.ipfreq);
                }
                if (pm_test.armfreq < 0 || pm_test.ahbfreq > 133000000 || pm_test.ipfreq > 66500000) {
                        printf("Please enter a valid frequency value \n");
                        printf("AHB and IP values must be less than maximum, 133MHz and 66.5MHz\n");
                        return -1;
                }
                printf("You have requested %d Hz ARM Core frequency\n", pm_test.armfreq);
                ioctl(fp, MXCTEST_PM_INTSCALE, &pm_test);
                break;

        case 2:
		   
                printf("This option is not supported on MXC27530 or MX27\n");
                printf("Use option 3 to test DVFS\n");
                printf("Enter a valid ARM frequency: \n");
                scanf("%d", &pm_test.armfreq);
                /*
                 * If someone enters just the MHz value, conver to Hz and
                 * assume they want ahb and ipg to be 133, 66.5 MHz.
                 */
                if ( pm_test.armfreq < 1000000 ) {
                        pm_test.armfreq *= 1000000;
				if((*platform==mxc91131) || (*platform==mx31))
					{
                        		pm_test.ahbfreq = 133000000;
                        		pm_test.ipfreq =  66500000;
					}
                } else {
                        printf("Enter a valid AHB frequency: \n");
                        scanf("%d", &pm_test.ahbfreq);
                        printf("Enter a valid IPG frequency: \n");
                        scanf("%d", &pm_test.ipfreq);
                }
                if (pm_test.armfreq < 0 || pm_test.ahbfreq > 133000000 || pm_test.ipfreq > 66500000) {
                        printf("Please enter a valid frequency value \n");
                        printf("AHB and IP values must be less than maximum, 133MHz and 66.5MHz\n");
                        return -1;
                }
                printf("You have requested %d Hz ARM Core frequency\n", pm_test.armfreq);
                ioctl(fp, MXCTEST_PM_PLLSCALE, &pm_test);

                break;

        case 3:
                printf("Enter a valid ARM frequency: \n");
                scanf("%d", &pm_test.armfreq);
                /*
                 * If someone enters just the MHz value, conver to Hz and
                 * assume they want ahb and ipg to be 133, 66.5 MHz.
                 */
                if ( pm_test.armfreq < 1000000 ) {
                        pm_test.armfreq *= 1000000;
                        pm_test.ahbfreq = 133000000;
                        pm_test.ipfreq =  66500000;                                
                } else {
                        printf("Enter a valid AHB frequency: \n");
                        scanf("%d", &pm_test.ahbfreq);
                        printf("Enter a valid IPG frequency: \n");
                        scanf("%d", &pm_test.ipfreq);
                }
                if (pm_test.armfreq < 0 || pm_test.ahbfreq > 133000000 || pm_test.ipfreq > 66500000) {
                        printf("Please enter a valid frequency value \n");
                        printf("AHB and IP values must be less than maximum, 133MHz and 66.5MHz\n");
                        return -1;
                }
                printf("You have requested %d Hz ARM Core frequency\n", pm_test.armfreq);
                ioctl(fp, MXCTEST_PM_INT_OR_PLL, &pm_test);
                break;
        case 4:
                printf("1. WAIT mode \n");
                printf("2. DOZE mode \n");
                printf("3. STOP mode \n");
                printf("4. DSM mode \n");
                printf("Enter a valid choice: \n");
                scanf("%d", &pm_test.lpmd);
                printf("Testing low-power modes\n");
                ioctl(fp, MXCTEST_PM_LOWPOWER, &pm_test);
                break;
        case 5:
                printf("Enter CKOH choice (1,2,3): \n");
                printf("1. ARM Clock \n");
                printf("2. AHB Clock \n");
                printf("3. IP Clock \n");
                scanf("%d", &pm_test.ckoh);
                switch( pm_test.ckoh ) {
                case 1:
                        printf("Setting CK_OK output to AP ARM clock divided by 10.\n");
                        pm_test.ckoh = CKOH_AP_SEL;
                        ioctl(fp, MXCTEST_PM_CKOH_SEL, &pm_test);
                        break;

                case 2:
                        printf("Setting CK_OK output to AP AHB clock divided by 10.\n");
                        pm_test.ckoh = CKOH_AHB_SEL;
                        ioctl(fp, MXCTEST_PM_CKOH_SEL, &pm_test);
                        break;

                case 3:
                        printf("Setting CK_OK output to AP IP clock.\n");
                        pm_test.ckoh = CKOH_IP_SEL;
                        ioctl(fp, MXCTEST_PM_CKOH_SEL, &pm_test);
                        break;

                default:
                        printf("Invalid choice\n");
                        break;
                }

                break;

       case 6:
                ioctl(fp, MXCTEST_PM_PMIC_HIGH, &pm_test);
                break;
        case 7:
                ioctl(fp, MXCTEST_PM_PMIC_LOW, &pm_test);
                break;
        case 8:
                ioctl(fp, MXCTEST_PM_PMIC_HILO, &pm_test);
                break;
                printf("Enter a random seed: \n");
                scanf("%d", &seed);

                srand(seed);

                /*
                 * Determine how many operating points we have:
                 * Assuming it is 4 or less
                 */
                for ( op_pt_ctr = 0 ;
                      (op_pt_ctr < 4) && (op_points[op_pt_ctr]!=0) ;
                      op_pt_ctr++ );

                /* current op point */
                pm_test.armfreq = 399000000;
                pm_test.ahbfreq = 133000000;
                pm_test.ipfreq  =  66500000;

                /* History of last 4 op points */
                point3 = point2 = point1 = 0;
                point0 = pm_test.armfreq;

                pctr0 = pctr1 = pctr2 = pctr3 = 0;
                while (1) {
                        /*
                         * get a random number from rand().  It's funny how
                         * some bits are more random than others.
                         */
                        random_op_pt = (rand()>>6) & 0x3;

                        /* toss away random points that are >= op_pt_ctr */
                        if (random_op_pt >= op_pt_ctr) {
                                continue;
                        }

                        new_op_point = (op_points[random_op_pt] * 1000000);

                        pm_test.armfreq = new_op_point;

                        point3 = point2;
                        point2 = point1;
                        point1 = point0;
                        point0 = pm_test.armfreq;

                        // if( pm_test.armfreq == 133000000 ) pctr0++;
                        // if( pm_test.armfreq == 266000000 ) pctr1++;
                        // if( pm_test.armfreq == 399000000 ) pctr2++;
                        // if( pm_test.armfreq == 532000000 ) pctr3++;
                        // printf("%d : %d %d %d %d\n", pm_test.armfreq,
                        //         pctr0, pctr1, pctr2, pctr3 );

                        // printf("Requesting %d Hz\n", pm_test.armfreq);
                        ioctl(fp, MXCTEST_PM_INT_OR_PLL, &pm_test);
                        printf("\n");
                        for ( i=0 ; i<1000000 ; i++ ) {};
                        printf("Sequence: %d -> %d -> %d -> %d\n",
                                point3/1000000,
                                point2/1000000,
                                point1/1000000,
                                point0/1000000 );
                        for ( i=0 ; i<1000000 ; i++ ) {};

                }
                break;

        default:
        printf("Invalid choice\n");
        }

        return 0;
}

