/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * This tests the Freescale MXC Security driver for the basic operation.
 * The following tests are done.
 * - IOCTL tests
 *      - Configure RNGA.
 *      - Generate Random number.
 *      - Generate N Random numbers.
 *      - Set entropy (Initial Seed for generating random numbers).
 *      - Status of RNGA.
 *      - Number of data availability in FIFO.
 *      - Size of the FIFO.
 *      - Hash the data. 
 *      - Status of HAC Hash.
 *      - Configure RTIC. 
 */

/*
 * This # define is used if debugging is required
 * #define  SEC_TEST_DEBUG
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <asm/ioctls.h>
#include <asm/termios.h>
#include <errno.h>
#include <malloc.h>
#include "mxc_test.h"

int ioctl(int fd, int cmd, void *arg);
int close(int fd);

int main(int ac, char *av[])
{
        int fd, ret_val, option, hash_algo, mem_select,sel;
        unsigned int arg,*temp_arg[4], i;
        char *defdev="/dev/mxc_test";
#ifdef  SEC_TEST_DEBUG
        printf("SEC TEST APP: In function main\n\r");
        printf("SEC TEST APP: To open the FD of Security\n\r");
#endif
        fd = open(defdev, O_RDONLY);
        if(fd < 0){
                printf("SEC TEST APP: Open failed with err %d\n", fd);
                perror("Open");
                return -1;
        }
#ifdef  SEC_TEST_DEBUG
        printf("SEC TEST APP: Open succes: retval %d\n", fd);
#endif
//        while (1) {
                printf("========> Testing MXC security driver <========\n");
                printf("Select the following security module to be tested: \n");
                printf("1. HAC \n");
                printf("2. RTIC \n");
                printf("3. Exit \n");
                printf("Enter your option: ");
//                scanf("%d", &option);
		printf("av=%s  %s\n",*(av+0),*(av+1));
		option =atoi(*(av+1));
		printf("Option =%d\n",option);
                switch(option) {
                case 1: /* HAC */
			sel = 2;
                        while (1) {
                        printf("======> Testing MXC HAC driver <======\n");
                        printf("1. Hash the data.\n");
                        printf("2. Get Hash status\n");
                        printf("3. Exit\n");
                        printf("Enter your option: ");
			option =atoi(*(av+sel));
                        
                        switch (option)
                        {
                                case 1:
                                printf("HAC TEST APP: Start hashing.\n");
                                /* Address of the data to be hashed */
                                temp_arg[0] = (unsigned int *)malloc\
                                                (sizeof(unsigned int) * 0x10);
                                /* Block length that needs to be hashed. 512
                                 * bits per block */
                                temp_arg[1] = (unsigned int *)1;
                                /* Address of the data to be hashed */
                                temp_arg[2] = (unsigned int *)malloc\
                                                (sizeof(unsigned int) * 0x10);
                                /* Block length that needs to be hashed. 512
                                 * bits per block */
                                temp_arg[3] = (unsigned int *)1;
                                
                                for (i = 0; i < 0x10; i++) {
                                        *(temp_arg[0] + i) = i + 0x12345678;
                                        *(temp_arg[2] + i) = i + 0x87654321;
                                }
                                
                                printf("HAC TEST APP: Data in block1:\n");
                                for (i = 0; i < 0x10; i++) {
                                        printf("0x%08X ", temp_arg[0][i]);
                                }
                                        
                                printf("\nHAC TEST APP: Data in block2:\n");
                                for (i = 0; i < 0x10; i++) {
                                        printf("0x%08X ", temp_arg[2][i]);
                                }
                                printf("\nParameters to be sent to driver:\n");
                                printf("HAC TEST APP: Parameter1: 0x%08X\n",\
                                        *(unsigned int *)temp_arg);
                                printf("HAC TEST APP: Parameter2: 0x%08X\n",\
                                        *(unsigned int *)(temp_arg + 1));
                                printf("HAC TEST APP: Parameter3: 0x%08X\n",\
                                        *(unsigned int *)(temp_arg + 2));
                                printf("HAC TEST APP: Parameter4: 0x%08X\n",\
                                        *(unsigned int *)(temp_arg + 3));
                                
                                ret_val = ioctl(fd, MXCTEST_HAC_START_HASH,
                                                 temp_arg);
                                if (ret_val < 0) {
                                        printf("HAC TEST APP: ioctl call fail"
                                                "ed with err %d\n", ret_val);
                                        perror("ioctl");
                                        return -1;
                                }
                                printf("\n==================================="
                                        "============\n");
                                /* Freeing the memory that was allocated */
                                free(temp_arg[0]);
                                free(temp_arg[2]);

                                break;

                                case 2:
                                printf("HAC TEST APP: Display Hash status\n");
                                ret_val = ioctl(fd, MXCTEST_HAC_STATUS, &arg);
                                if (ret_val < 0) {
                                        printf("HAC TEST APP: ioctl call fail"\
                                                "ed with err %d\n", ret_val);
                                        perror("ioctl");
                                        return -1;
                                }
				else {
					printf("TEST PASSED\n");
					return 0;
				}
                                printf("\n==================================="
                                        "============\n");

                                break;
                                
                                case 3:
                                default:
                                        goto hac_out;
                                break;
                         
                        }/* End of HAC Switch case statement */
			sel++;
                        }/* End of HAC while */
hac_out:                
                break; /* End of HAC case */

                case 2: /* RTIC */
			sel = 2;
                        while (1) {
                        printf("======> Testing MXC RTIC driver <======\n");
			printf("1. Select the Hash Algorithm\n");
			printf("2. Configure the Memory Block\n");
                        printf("3. ONE TIME HASH.\n");
                        printf("4. Select Memory for RUN TIME\n");
			printf("5. Write Hash Register\n");
                        printf("6. RUN TIME HASH.\n");
                        printf("7. Check for error condition during Run Time."\
                                "\n");
                        printf("8. Get Hash status\n");
                        printf("9. Exit\n");
                        printf("Enter your option: ");
               //         scanf("%d",&option);
			option =atoi(*(av+sel));
                        switch (option)
                        {
                                
				case 1: 
				printf("Select the Hash Algorithm \n");
                                printf("SHA-1   select 0\n");
                                printf("SHA-256 select 1\n");
                              //  scanf("%d",&hash_algo);
				sel++;
				hash_algo = atoi(*(av+sel));
				temp_arg[0]=(unsigned int *)hash_algo;
                                ret_val = ioctl(fd, MXCTEST_RTIC_ALGOSELECT,temp_arg);
                                if (ret_val < 0) {
                                        printf("RTIC TEST APP: ioctl call fail"
                                                "ed with err %d\n", ret_val);
                                        perror("ioctl");
                                        return -1;
                                }
				break;
				case 2:
                                printf("RTIC TEST APP: Start Configuring the Memory.\n");
                                /* Address of the data to be hashed */
                                temp_arg[0] = (unsigned int *)malloc\
                                                (sizeof(unsigned int) * 0x10);
                                /* Block length that needs to be hashed. 512
                                 * bits per block */
                                temp_arg[1] = (unsigned int *)0x40;
                                printf("0. MEMORY A1\n");
                                printf("1. MEMORY A2\n");
                                printf("2. MEMORY B1\n");
                                printf("3. MEMORY B2\n");
                                printf("4. MEMORY C1\n");
                                printf("5. MEMORY C2\n");
                                printf("6. MEMORY D1\n");
                                printf("7. MEMORY D2\n");
                       //         scanf("%d",&mem_select);
				sel++;
				mem_select = atoi(*(av+sel));
                                temp_arg[2]= (unsigned int *)mem_select;
                                for (i = 0; i < 0x10; i++) {
                                        *(temp_arg[0] + i) = (mem_select+1)* i + 0x12345678;
                                }

                                printf("RTIC TEST APP: Data in block1:\n");
                                for (i = 0; i < 0x10; i++) {
                                        printf("0x%08X  ", temp_arg[0][i]);
					if (i == 7)
						printf("\n");
                                }
                                printf("\nLength of the Data\n");
                                printf("RTIC TEST APP: Parameter2: 0x%08X\n",\
                                        *(unsigned int *)(temp_arg + 1));

                                ret_val = ioctl(fd, MXCTEST_RTIC_CONFIGURE_MEMORY,
                                                 temp_arg);
                                if (ret_val < 0) {
                                        printf("RTIC TEST APP: ioctl call fail"
                                                "ed with err %d\n", ret_val);
                                        perror("ioctl");
                                        return -1;
                                }
                                printf("\n==================================="
                                        "============\n");
                                /* Freeing the memory that was allocated */
                                free(temp_arg[0]);
                                break;
				case 3:
				printf("Start ONE TIME Hashing\n");
				ret_val = ioctl(fd, MXCTEST_RTIC_ONETIME_TEST,
                                                 temp_arg);
                                if (ret_val < 0) {
                                        printf("RTIC TEST APP: ioctl call fail"
                                                "ed with err %d\n", ret_val);
                                        perror("ioctl");
                                        return -1;
                                }
                                printf("\n==================================="
                                        "============\n");
				break;
                                case 4:
				printf("Select Memory for RUN TIME Hash\n");
				printf("0. MEMORY A\n");
                               	printf("2. MEMORY B\n");
                               	printf("4. MEMORY C\n");
                               	printf("6. MEMORY D\n");
                       //      	scanf("%d",&mem_select);
				sel++;
				mem_select = atoi(*(av+sel));
				temp_arg[0]= (unsigned int *)mem_select;

				ret_val = ioctl(fd,  MXCTEST_RTIC_CONFIGURE_RUNTIME_MEMORY,
                                                 temp_arg);
                                if (ret_val < 0) {
                                        printf("RTIC TEST APP: ioctl call fail"
                                                "ed with err %d\n", ret_val);
                                        perror("ioctl");
                                        return -1;
				}
				 printf("\n==================================="
                                        "============\n");
				break;
				case 5:
                                printf("Select Memory After WARM Boot\n");
                                printf("0. MEMORY A1\n");
                                printf("1. MEMORY A2\n");
                                printf("2. MEMORY B1\n");
                                printf("3. MEMORY B2\n");
                                printf("4. MEMORY C1\n");
                                printf("5. MEMORY C2\n");
                                printf("6. MEMORY D1\n");
                                printf("7. MEMORY D2\n");
                //                scanf("%d",&mem_select);
                                sel++;
                                mem_select = atoi(*(av+sel));
                                temp_arg[0]= (unsigned int *)mem_select;

                                ret_val = ioctl(fd,  MXCTEST_RTIC_CONFIGURE_RUNTIME_MEMORY_AFTER_WARM_BOOT,
                                                 temp_arg);
                                if (ret_val < 0) {
                                        printf("RTIC TEST APP: ioctl call fail"
                                                "ed with err %d\n", ret_val);
                                        perror("ioctl");
                                        return -1;
                                }
                                 printf("\n==================================="
                                        "============\n");
				break;
                                case 6: 
				printf("Start RUN TIME HASH\n");
				ret_val = ioctl(fd, MXCTEST_RTIC_RUNTIME_TEST,
                                                 temp_arg);
                                if (ret_val < 0) {
                                        printf("RTIC TEST APP: ioctl call fail"
                                                "ed with err %d\n", ret_val);
                                        perror("ioctl");
                                        return -1;
                                }
                                printf("\n==================================="
                                        "============\n");
                                break;

                                case 7:
                                /* Address of the data to be hashed */
                                temp_arg[0] = (unsigned int *)malloc\
                                                (sizeof(unsigned int) * 0x10);
                                /* Block length that needs to be hashed. 512
                                 * bits per block */
                                temp_arg[1] = (unsigned int *)0x10;

                                for (i = 0; i < 0x10; i++) {
                                        *(temp_arg[0] + i) = i + 0x12345678;
                                }

                                printf("RTIC TEST APP: Data in block1:\n");
                                for (i = 0; i < 0x10; i++) {
                                        printf("0x%08X  ", temp_arg[0][i]);
					if ( i == 7)
						printf("\n");
                                }
				printf("\n");
                                printf("RTIC TEST APP: Parameter2: 0x%08X\n",\
                                        *(unsigned int *)(temp_arg + 1));
                                ret_val = ioctl(fd, MXCTEST_RTIC_RUNTIME_ERROR,
                                                 temp_arg);
                                if (ret_val < 0) {
                                        printf("RTIC TEST APP: ioctl call fail"
                                                "ed with err %d\n", ret_val);
                                        perror("ioctl");
                                        return -1;
                                }else
					printf("\nRTIC_TEST_PASSED\n");
				
                                printf("\n==================================="
                                        "============\n");
                                /* Freeing the memory that was allocated */
                                //free(temp_arg[0]);
                                break;
                                        
                                case 8:
                                printf("RTIC TEST APP: Display Hash status\n");

                                ret_val = ioctl(fd, MXCTEST_RTIC_STATUS, &arg);
                                if (ret_val < 0) {
                                        printf("RTIC TEST APP: ioctl call fail"
                                                "ed with err %d\n", ret_val);
                                        perror("ioctl");
                                        return -1;
                                }
				else
					printf ("RTIC_TEST_PASSED\n");
                                printf("\n==================================="
                                        "============\n");
                                break;

                                case 9:
                                default:
                                        goto rtic_out;
                                break;
                         
                        }/* End of RTIC Switch case statement */
		sel++;
                        }/* End of RTIC while loop */
rtic_out:                        
                break; /* End of RTIC case */

                case 3:
                default: /* Default of main switch statement */
                        goto out_main;
                break; /* End of exit */

                } /* End of main switch statement */
    //    } /* End of External while loop */

out_main:
        printf("SEC TEST APP: Closing the security device \n");
        ret_val = close(fd);
        if(ret_val < 0)  {
                printf("SEC TEST APP: Close failed with err %d\n", ret_val);
                perror("Close");
                return -1;
        }
        printf("SEC TEST APP: Close success: retval %d\n", ret_val);
        printf("SEC TEST APP: Testing MXC security driver complete\n");
        return 0;
}

