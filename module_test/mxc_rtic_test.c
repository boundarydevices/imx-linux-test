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
/* Setting the watch dog timer value */
#define wtch_timer  0x3E8

/*
 * General Include Files
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>
#include "../include/mxc_test.h"
/*
 * Driver specific include files
 */
#include <asm/arch/mxc_security_api.h>

ulong *gtempu32ptr;
static struct class *mxc_test_class;
static int hash_algo;
/* Hash Values of Various Memory for SHA1/SHA256 for Verification */
ulong hash_correct_A1_256[]={0xBC1E371E,0xE8A977EA, 0xF954E8A4,
				0xC000DD1A,0x5E81C57C,0x6826FDD2,
						0x66F24B7A,0xDBA20BC5};

ulong hash_correct_A2_256[]={0x1A13C7F0,0x5EEA9445,0xA523EC2C,
				0xCFE7A461,0x63398A2B,0xA2B2DF63,
						0x200C7269,0xB2417B93};
ulong hash_correct_A1A2_256[]={0x6D08878B,0xC1449ED6,0x21A5E71D,
				0x5EA40785,0x96718C77,0x769D4F52,
						0xA48A8708,0xFE19A4D2};
long hash_correct_A1_160[]={0x15BC691A,0x33354AA2,0xE748FE05,
					0xBE0F7E08,0xC08A101C};
ulong hash_correct_A2_160[]={0x934E092E,0xC0FA16B4,0x0E54E689,
					0x3E7A39B0,0xBA263368};
ulong hash_correct_A1A2_160[]={0x79A32B75,0xD9841A63,0xFD818BAB,
					0xE551D7E5,0xC2186F66};
ulong hash_correct_B1_256[]={0x472FB397,0x6B03DEB1,0x35DB8F48,
                                0x1189725B,0xAE28D13F,0xDC7D8D12,
                                                0x1FDCE982,0x459D038D};
ulong hash_correct_B2_256[]={0xDA309008,0x819E57C9,0xA51AD140,
                                0xCB87ACF5,0x55D36353,0x8F37F3CC,
                                                0x41F108A6,0xB5C76D5D};

ulong hash_correct_B1B2_256[]={0x94510FB1,0xF54961B1,0xF1F65750,
				0x036BDB95,0x91211D29,0xB661FE79,
                                                0x258A1983,0x149899C9};
ulong hash_correct_B1_160[]={0x6940306D,0xC8BCBDAE,0x17276B2D,
                                           0x89D74834,0x994006C3};

ulong hash_correct_B2_160[]={0x6D7D2DFD,0xB662929C,0x2FF96C78,
					0xB7894885,0xD8EAFAD8};
ulong hash_correct_B1B2_160[]={0x2DC19C7B,0x3911EBED,0xA4B4ADB3,
					0xFFF59C47,0xD4C90D52};

ulong hash_correct_C1_256[]={0x1CD695FE,0x0D22DC89,0x7275C153,
				0xD1F0F8F9,0x5CA47FB2,0x637ADCEB,
					0x2D3697FB,0xC8818361};
ulong hash_correct_C2_256[]={ 0x64E877D1,0xCAA0B96B,0xA145660B,
				0xEE433609,0x5A2515B2,0x27924962,
					0x961BD65B,0xC17F67C4};
ulong hash_correct_C1C2_256[]={0x45B6A050,0x42B40B04,0x58DDED03,
				0xFD0BACB5,0x3AF4E09D,0x57995C10,
					0x432ED9A9,0x74D2FB24};

ulong hash_correct_C1_160[]={0xF13507EF,0x5783FB1D,0x1D64380F,
				0x43D66C47,0xF2FCD88F};
ulong hash_correct_C2_160[]={0xFFB6B71B,0xFCCEEC9A,0xC70B21B8,
				0x6A4DE926,0x9157E32E};
ulong hash_correct_C1C2_160[]={0xE5BF5A6A,0x9258B9D2,0x74EF5253,
				0x89ABB800,0xDA359C1F};

ulong hash_correct_D1_256[]={0x1941454C,0xA3DB1FD8,0xEF1B2F6E,
				0xAC8FAD56,0x29454873,0x399F4E33,
					0x02E52999,0x798C8410};

ulong hash_correct_D2_256[]={0x4A2668BA,0x9BD68E74,0xF5230B7F,
				0x6B6395E1,0x25DB8236,0x75DA2A4C,
					0x44CB08DB,0xC8E16F6A};
ulong hash_correct_D1D2_256[]={0x4BADA52A,0x9D3BB003,0x22DC4207,
				0x52D954A9,0xFD8E20ED,0x50D7CF7D,
					0x4B5235A8,0x8CF7BF97};
ulong hash_correct_D1_160[]={0xBB754FCD,0x02178289,0x506CA40D,
				0x6F4B0CE6,0x361BA2DC};

ulong hash_correct_D2_160[]={0xEAE94800,0x2B190DA5,0xC2F039FC,
				0x9D71323C,0x26BD2FC2};
ulong hash_correct_D1D2_160[]={0xC542DD97,0x4BB92FBB,0xFE0E603C,
				0x611F99F7,0xE55BA1F4};

static int mxc_test_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t mxc_test_read(struct file *file, char *buf, size_t count,
			     loff_t * ppos)
{
	return 0;
}

static ssize_t mxc_test_write(struct file *filp, const char *buf, size_t count,
			      loff_t * ppos)
{
	return 0;
}
rtic_ret authenticate_hash_value(rtic_hash_rlt *hash_result_reg) {
	ulong rtic_control;
	static int checkA, checkB, checkC;
	rtic_control = rtic_get_control();
	if( ((rtic_control & 0x10) == 0x10) && !checkA) {
		if((rtic_control & 0x1000) == 0x1000) {
                        if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_A1_256,32))){
                                printk("Hash values are Authenticated\n");
                        }
                        else if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_A2_256,32))){
                                printk ("Hash Values are Authenticated\n");
                        }
                        else if (!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_A1A2_256,32))) {
                                printk("Hash Values are Authenticated\n");
                        }
                        else {
                                printk("Hash Values are wrong\n");
				
			}
		}
		else {
			if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_A1_160,20))){
				printk("Hash values are Authenticated\n");
			}
			else if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_A2_160,20))){
				printk ("Hash Values are Authenticated\n");
				
			}
			else if (!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_A1A2_160,20))) {
				printk("Hash Values are Authenticated\n");
				
			}
			else {
				printk("Hash Values are wrong\n");
				
			}
		}
		checkA = 1;		
	}
	else if(((rtic_control & 0x20) == 0x20) && !checkB) {
		if((rtic_control & 0x2000) == 0x2000) {
                        if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_B1_256,32))){
                                printk("Hash values are Authenticated\n");
			
                        }
                        else if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_B2_256,32))){
                                printk ("Hash Values are Authenticated\n");
				
                        }
                        else if (!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_B1B2_256,32))) {
                                printk("Hash Values are Authenticated\n");
			
                        }
                        else {
                                printk("Hash Values are wrong\n");
			}
		}
		else {
                        if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_B1_160,20))){
                                printk("Hash values are Authenticated\n");
                        }
                        else if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_B2_160,20))){
                                printk ("Hash Values are Authenticated\n");
                        }
                        else if (!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_B1B2_160,20))) {
                                printk("Hash Values are Authenticated\n");
                        }
                        else {
                                printk("Hash Values are wrong\n");
			}
		}
		checkB = 1;	
	}
	else if((rtic_control & 0x40) == 0x40 && !checkC) {
		if((rtic_control & 0x4000) == 0x4000) {
                        if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_C1_256,32))){
                                printk("Hash values are Authenticated\n");
			
                        }
                        else if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_C2_256,32))){
                                printk ("Hash Values are Authenticated\n");
			
                        }
                        else if (!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_C1C2_256,32))) {
                                printk("Hash Values are Authenticated\n");
				
                        }
                        else {
                                printk("Hash Values are wrong\n");
				
			}
		}
		else {
                        if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_C1_160,20))){
                                printk("Hash values are Authenticated\n");
			
                        }
                        else if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_C2_160,20))){
                                printk ("Hash Values are Authenticated\n");
				
                        }
                        else if (!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_C1C2_160,20))) {
                                printk("Hash Values are Authenticated\n");
				
                        }
                        else {
                                printk("Hash Values are wrong\n");
				
			}
		}
		checkC = 1;		
	}
	else  { 
		if((rtic_control & 0x8000) == 0x8000) {
                        if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_D1_256,32))){
                                printk("Hash values are Authenticated\n");
                        }
                        else if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_D2_256,32))){
                                printk ("Hash Values are Authenticated\n");
                        }
                        else if (!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_D1D2_256,32))) {
                                printk("Hash Values are Authenticated\n");
                        }
                        else {
                                printk("Hash Values are wrong\n");
			}
		}
		else {
                       if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_D1_160,20))){
                                printk("Hash values are Authenticated\n");
                        }
                        else if(!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_D2_160,20))){
                                printk ("Hash Values are Authenticated\n");
                        }
                        else if (!(memcmp((void *)hash_result_reg->hash_result,
				(void *)&hash_correct_D1D2_160 ,20))) {
                                printk("Hash Values are Authenticated\n");
                        }
                        else {
                                printk("Hash Values are wrong\n");
			}
		}
	}
	return 0;	
}
static int mxc_test_ioctl(struct inode *inode, struct file *file,
			  unsigned int cmd, unsigned long arg)
{
	ulong *tempu32ptr, tempu32;

	ulong rtic_length1 = 0x01,i = 0;
	static int hash_algoA,hash_algoB,hash_algoC,hash_algoD,count1;
	static ulong rtic_lengthA1, rtic_lengthA2, rtic_lengthB1, rtic_lengthB2,
		rtic_lengthC1, rtic_lengthC2, rtic_lengthD1, rtic_lengthD2;
	ulong rtic_memdata2, rtic_rem, rtic_stat;
	static ulong rtic_memdata2A1, rtic_memdata2A2, rtic_memdata2B1, rtic_memdata2B2,
		rtic_memdata2C1, rtic_memdata2C2, rtic_memdata2D1, rtic_memdata2D2;
	static ulong *rtic_memdata1;
	static ulong *rtic_memdataA1, *rtic_memdataA2, *rtic_memdataB1, 
	       *rtic_memdataB2, *rtic_memdataC1, *rtic_memdataC2, *rtic_memdataD1, *rtic_memdataD2;
	ulong rtic_config, rtic_run_config, rtic_config_mode, rtic_int,
	    rtic_result, wd_time_set,temp,temp1,temp2;
	static rtic_hash_rlt rtic_res[4];
	int mem_select, count, loop, hash_number;
	unchar irq_en = 0x01;
	tempu32 = (ulong) (*(ulong *) arg);
	tempu32ptr = (ulong *) arg;
	/* Initializing the RTIC clk */
	rtic_init();
	switch (cmd) {

	case MXCTEST_RTIC_ALGOSELECT:
		if(*(tempu32ptr + 0))
			hash_algo = 1;
		else
			hash_algo = 0;
		return 0;	
	case MXCTEST_RTIC_CONFIGURE_MEMORY:
                rtic_stat = rtic_get_status();
                if((rtic_stat & 0x01) == 0x01) {
                        printk("Error : RTIC IS BUSY\n");
                        return -1;
                }
		mem_select = *(tempu32ptr + 2);
                rtic_length1 = *(tempu32ptr + 1);
                if ((rtic_rem = ((ulong) rtic_length1 % 4)) != 0) {
                        rtic_length1 = ((ulong) rtic_length1 + 4 - rtic_rem);
                }
                /* Allocating memory to copy user data to kernel memory */
		rtic_memdata1 =(ulong *) dma_alloc_coherent(NULL, 
				(sizeof(ulong) * (rtic_length1 + 1)),
				(dma_addr_t *)&rtic_memdata2, GFP_DMA);
		
		if(rtic_memdata1 == NULL){
			printk("Failed to Allocate Memory\n");
			return  ENOMEM;
		}
		if(mem_select == 0) {
			rtic_memdataA1 = rtic_memdata1;
			rtic_memdata2A1 = rtic_memdata2;
			rtic_lengthA1 = rtic_length1;
			hash_algoA = hash_algo;
		}
		else if(mem_select == 1) {
		        rtic_memdataA2 = rtic_memdata1;
                        rtic_memdata2A2 = rtic_memdata2;
			rtic_lengthA2 = rtic_length1;
		}
		else if(mem_select == 2) {
                        rtic_memdataB1 = rtic_memdata1;
                        rtic_memdata2B1 = rtic_memdata2;
			rtic_lengthB1 = rtic_length1;
			hash_algoB = hash_algo;
		}
		else if(mem_select == 3) {
                        rtic_memdataB2 = rtic_memdata1;
                        rtic_memdata2B2 = rtic_memdata2;
			rtic_lengthC1 = rtic_length1;
		}
		else if(mem_select == 4) {
                        rtic_memdataC1 = rtic_memdata1;
                        rtic_memdata2C1 = rtic_memdata2;
			rtic_lengthC1 = rtic_length1;
			hash_algoC = hash_algo;
		}
		else if(mem_select == 5) {
                        rtic_memdataC2 = rtic_memdata1;
                        rtic_memdata2C2 = rtic_memdata2;
			rtic_lengthC2 = rtic_length1;
		}
		else if(mem_select == 6) {
                        rtic_memdataD1 = rtic_memdata1;
                        rtic_memdata2D1 = rtic_memdata2;
			rtic_lengthD1 = rtic_length1;
			hash_algoD = hash_algo;
		}
		else {
			rtic_memdataD2 = rtic_memdata1;
			rtic_memdata2D2 = rtic_memdata2;
			rtic_lengthD2 = rtic_length1;
		}
                /* Physical address 64 byte alignment */
                if ((rtic_rem = ((ulong) rtic_memdata2 % 4)) != 0) {
                        rtic_memdata2 = ((ulong) rtic_memdata2 + 4 - rtic_rem);
                }

                /* Copying 1st memory block from user space to kernel space */
                /* 1 block of data is 512 bytes */
                i = copy_from_user(rtic_memdata1, (ulong *) * tempu32ptr,
                                   rtic_length1 * 4);
                if (i != 0) {
                        RTIC_DEBUG("RTIC TEST DRV: Couldn't copy %lu bytes of"
                                   "data from user space\n", i);
                        return -EFAULT;
                }
                tempu32 = rtic_length1;
                for (i = 0; i < tempu32; i++) {
                        RTIC_DEBUG("x%08lX \t ", *(rtic_memdata1 + i));
                }
		/* Configure for start address and blk length */
                rtic_config = rtic_configure_mem_blk((ulong) rtic_memdata2,
                                                     rtic_length1, mem_select, hash_algo);
                if (rtic_config == RTIC_SUCCESS) {
                        RTIC_DEBUG("RTIC TEST DRV: Configuring memory block "
                                   "of RTIC is success.\n");
                } else if (rtic_config == RTIC_FAILURE) {
                        RTIC_DEBUG("RTIC TEST DRV: Configuring memory block "
                                   "of RTIC is failure.\n");
			kfree(rtic_memdata1);
			kfree(tempu32ptr);
                }
                return 0;
	case MXCTEST_RTIC_ONETIME_TEST:
		/* DMA Throttle reg is set to 0x00 during boot time mode. */
		rtic_hash_once_dma_throttle();
		/* Configuring the ONE-TIME Hashing */
		mem_select = 0;
		rtic_config_mode = rtic_configure_mode(RTIC_ONE_TIME, mem_select);
		if (rtic_config_mode == RTIC_SUCCESS) {
			RTIC_DEBUG("RTIC TEST DRV: Configuring "
				   "of RTIC is success.\n");
		} else if (rtic_config_mode == RTIC_FAILURE) {
			RTIC_DEBUG("RTIC TEST DRV: Configuring  "
				   "of RTIC is failure.\n");
			kfree(rtic_memdata1);
		}
		rtic_wd_timer(wtch_timer);
		/* Enable Interrupt */
		rtic_int = rtic_configure_interrupt(irq_en);
		if (rtic_int == RTIC_SUCCESS) {
			RTIC_DEBUG("RTIC TEST DRV: Configuring "
				   "of RTIC interrupt is success.\n");
		} else if (rtic_int == RTIC_FAILURE) {
			RTIC_DEBUG("RTIC TEST DRV: Configuring  "
				   "of RTIC interrupt is failure.\n");
		}
		/* Start ONE TIME Hashing of the Selected Memory */
		rtic_start_hash(RTIC_ONE_TIME);
		printk
		    ("RTIC Control reg.0x%08lX\n",
		     rtic_get_control());
		if ((rtic_get_status() & 0x04) == RTIC_STAT_HASH_ERR) {
			RTIC_DEBUG("RTIC TEST DRV: RTIC Status register. "
				   "0x%08lX \n ", rtic_get_status());
			printk("RTIC TEST DRV: Encountered Error while "
			       "processing\n");
			/* Bus error has occurred */
			/* Freeing all the memory allocated */
			kfree(rtic_memdata1);
			return -EACCES;
		} else if ((rtic_get_status() & 0x02) == RTIC_STAT_HASH_DONE) {
			printk("RTIC TEST DRV: Hashing done. Continuing "
			       "hash.\n");
		}
		printk("RTIC TEST DRV: RTIC Status register.0x%08lX  \n",
		       rtic_get_status());

		/* Hashing done successfully. Now reading the  hash data
		 * from memory.
		 */
		temp = 1;
		temp1 = rtic_get_control();
                temp2 = temp1 >> 12;
		temp1 = temp1 >> 4;
		count = 0;
		loop = 1;
		while(loop) {
			if(temp1 & (temp << count)) {
				rtic_result = rtic_hash_result((count * 2), RTIC_ONE_TIME,
							       &rtic_res[count]);
				if (rtic_result == RTIC_SUCCESS) {
					RTIC_DEBUG("RTIC TEST DRV: Read Hash data from "
						   "RTIC is success.\n");
				} else if (rtic_result == RTIC_FAILURE) {
					RTIC_DEBUG("RTIC TEST DRV: Read Hash data from  "
						   "RTIC is failure.\n");
				}
		                 /* Verifying the Hash Value given by the Hardware */
                                authenticate_hash_value(&rtic_res[count]);

				if(temp2 & (temp << count))
					hash_number = 8;
				else
					hash_number = 5;
				/* Displaying the hash data */
				for (i = 0; i < hash_number; i++) {
					printk("RTIC TEST DRV: Hash result%lu: 0x%08lX\n",
					       i + 1, rtic_res[count].hash_result[i]);
				}
				printk("\n");
			}
			count++;
			if(count == 4)
				loop = 0;

		}
	      return 0;
	case MXCTEST_RTIC_CONFIGURE_RUNTIME_MEMORY:
		mem_select = *(tempu32ptr + 0);
		/* Configure Memory for RUN TIME Hashing */
                rtic_run_config = rtic_configure_mode(RTIC_RUN_TIME, mem_select);
		return 0;
	case MXCTEST_RTIC_CONFIGURE_RUNTIME_MEMORY_AFTER_WARM_BOOT:
                mem_select = *(tempu32ptr + 0);
                if(mem_select == 0) { 
                        rtic_memdata2 = rtic_memdata2A1;
			rtic_length1 = rtic_lengthA1;
			hash_algo = hash_algoA;
		}
                else if(mem_select == 1) { 
                        rtic_memdata2 = rtic_memdata2A2;
			rtic_length1 = rtic_lengthA2;
			hash_algo = hash_algoA;
		}
                else if(mem_select == 2) {
                        rtic_memdata2 = rtic_memdata2B1;
			rtic_length1 = rtic_lengthB1;
			hash_algo = hash_algoB;
		}
                else if(mem_select == 3) { 
                        rtic_memdata2 = rtic_memdata2B2;
			rtic_length1 = rtic_lengthB2;
			hash_algo = hash_algoB;
		}
                else if(mem_select == 4) {
                        rtic_memdata2 = rtic_memdata2C1;
			rtic_length1 = rtic_lengthC1;
			hash_algo = hash_algoC;
		}
                else if(mem_select == 5) {
                        rtic_memdata2 = rtic_memdata2C2;
			rtic_length1 = rtic_lengthC2;
			hash_algo = hash_algoC;
		}
                else if(mem_select == 6) {
                        rtic_memdata2 = rtic_memdata2D1;
			rtic_length1 = rtic_lengthD1;
			hash_algo = hash_algoD;
		}
                else {
                        rtic_memdata2 = rtic_memdata2D2;
			rtic_length1 = rtic_lengthD2;
			hash_algo = hash_algoD;
		}
                /* Configure for start address and blk length */
                rtic_config = rtic_configure_mem_blk((ulong) rtic_memdata2,
                                                     rtic_length1, mem_select, hash_algo);
                if (rtic_config == RTIC_SUCCESS) {
                        RTIC_DEBUG("RTIC TEST DRV: Configuring memory block "
                                   "of RTIC is success.\n");
                } else if (rtic_config == RTIC_FAILURE) {
                        RTIC_DEBUG("RTIC TEST DRV: Configuring memory block "
                                   "of RTIC is failure.\n");
                        kfree(rtic_memdata1);
                        kfree(tempu32ptr);
                }
		if(mem_select % 2 == 0) {
			if(hash_algo == 0)
                   		   hash_number = 5;
                         else
          	   		   hash_number = 8;
			/* Configure it to RUN Time mode  */
                	 rtic_configure_mode(RTIC_RUN_TIME, mem_select);
			/* Write the Hash value to Hash Register file */
			 rtic_hash_write(&rtic_res[count1], mem_select , hash_number);
			 count1++;
		}	
                return 0;
	case MXCTEST_RTIC_RUNTIME_TEST:
                /*Setting DMA Burst Read */
                rtic_dma_burst_read(RTIC_DMA_4_WORD);
               /* DMA Throttle reg is set to 0x00 during boot time mode. */
                rtic_hash_once_dma_throttle();
		/* Setting DMA delay */
                rtic_dma_delay(0x001f);
                /* Setting the Watch Dog Timer */
                wd_time_set = rtic_wd_timer(wtch_timer);
                if (wd_time_set == RTIC_SUCCESS) {
                        RTIC_DEBUG
                            ("RTIC TEST DRV : Configuring the watch dog timer"
                             "is success.\n");
                } else if (wd_time_set == RTIC_FAILURE) {
                        RTIC_DEBUG
                            ("RTIC TEST DRV : Configuring the watch dog timer"
                             " is failure \n");
                }
                printk
                    ("RTIC Control reg.0x%08lX\n",
                     rtic_get_control());
		/* Start RUN TIME Hashing for the Selected Memory */
		rtic_start_hash(RTIC_RUN_TIME);
		return 0;
	case MXCTEST_RTIC_RUNTIME_ERROR:
		rtic_length1 = *(tempu32ptr + 1);
		if ((rtic_rem = ((ulong) rtic_length1 % 4)) != 0) {
			rtic_length1 = ((ulong) rtic_length1 + 4 - rtic_rem);
		}
		/* Allocating memory to copy user data to kernel memory */
                rtic_memdata1 =(ulong *) dma_alloc_coherent(NULL,
                                (sizeof(ulong) * (rtic_length1 + 1)),
                                (dma_addr_t *)&rtic_memdata2, GFP_DMA);


		RTIC_DEBUG("RTIC TEST DRV: Virtual address 0x%08lX\n",
			   (ulong) rtic_memdata1);
		RTIC_DEBUG("RTIC TEST DRV: Physical address 0x%08lX\n",
			   rtic_memdata2);
		/* Physical address 64 byte alignment */
		if ((rtic_rem = ((ulong) rtic_memdata2 % 4)) != 0) {
			rtic_memdata2 = ((ulong) rtic_memdata2 + 4 - rtic_rem);
		}

		/* Copying 1st memory block from user space to kernel space */
		/* 1 block of data is 512 bytes */
		i = copy_from_user(rtic_memdata1, (ulong *) * tempu32ptr,
				   rtic_length1 * 4);
		if (i != 0) {
			RTIC_DEBUG("RTIC TEST DRV: Couldn't copy %lu bytes of"
				   "data from user space\n", i);
			return -EFAULT;
		}
		tempu32 = rtic_length1;
		for (i = 0; i < tempu32; i++) {
			RTIC_DEBUG("0x%08lX ", *(rtic_memdata1 + i));
		}

		/* Configure for start address and blk length */
		rtic_config = rtic_configure_mem_blk((ulong) rtic_memdata2,
						     rtic_length1 * 4, RTIC_A1, hash_algo);
		
		if (rtic_config == RTIC_SUCCESS) {
			RTIC_DEBUG("RTIC TEST DRV: Configuring memory block "
				   "of RTIC is success.\n");
		} else if (rtic_config == RTIC_FAILURE) {
			RTIC_DEBUG("RTIC TEST DRV: Configuring memory block "
				   "of RTIC is failure.\n");
			goto out;
		}

		/* Configuring the ONE-TIME Hashing */
		rtic_config_mode = rtic_configure_mode(RTIC_ONE_TIME,RTIC_A1);
		if (rtic_config_mode == RTIC_SUCCESS) {
			RTIC_DEBUG("RTIC TEST DRV: Configuring "
				   "of RTIC is success.\n");
		} else if (rtic_config_mode == RTIC_FAILURE) {
			RTIC_DEBUG("RTIC TEST DRV: Configuring  "
				   "of RTIC is failure.\n");
			goto out;
		}
		/* Enable Interrupt */
		rtic_int = rtic_configure_interrupt(irq_en);
		if (rtic_int == RTIC_SUCCESS) {
			RTIC_DEBUG("RTIC TEST DRV: Configuring "
				   "of RTIC interrupt is success.\n");
		} else if (rtic_int == RTIC_FAILURE) {
			RTIC_DEBUG("RTIC TEST DRV: Configuring  "
				   "of RTIC interrupt is failure.\n");
		}
		rtic_dma_burst_read(RTIC_DMA_16_WORD);
		/*Set DMA Delay between two DMA burst */
		rtic_dma_delay(0x001f);
		/* Set  WDtimer.\n"); */
		rtic_wd_timer(0x001f);
		/* Starting ONE_TIME hash */
		rtic_start_hash(RTIC_ONE_TIME);
		/* Configure Memory for RUN TIME Hash */
		rtic_run_config = rtic_configure_mode(RTIC_RUN_TIME, RTIC_A1);
		/*Set DMA delay between two DMA Burst */
		rtic_dma_delay(0x001f);
		rtic_wd_timer(0x0001f);
		/*Start RUN TIME hash */
		rtic_start_hash(RTIC_RUN_TIME);
		if ((rtic_get_status() & 0x04) == RTIC_STAT_HASH_ERR) {
			RTIC_DEBUG("RTIC TEST DRV: RTIC Status register. "
				   "0x%08lX \n ", rtic_get_status());
			printk("RTIC TEST DRV: Encountered Error while "
			       "processing\n");
			/* Bus error has occurred */
			/* Freeing all the memory allocated */
			kfree(rtic_memdata1);
			return -EACCES;
		} else if ((rtic_get_status() & 0x02) == RTIC_STAT_HASH_DONE) {
			printk("RTIC TEST DRV: Hashing done. Continuing "
			       "hash.\n");
		}
		rtic_result = rtic_hash_result(RTIC_A1, RTIC_RUN_TIME,
					       &rtic_res[0]);
		if (rtic_result == RTIC_SUCCESS) {
			RTIC_DEBUG("RTIC TEST DRV: Read Hash data from "
				   "RTIC is success.\n");
		} else if (rtic_result == RTIC_FAILURE) {
			RTIC_DEBUG("RTIC TEST DRV: Read Hash data from  "
				   "RTIC is failure.\n");
		}
		rtic_stat = rtic_get_status();
		if((rtic_stat & 0x8) == 0x8)
			return RTIC_SUCCESS;
		else
			return RTIC_FAILURE;

		/* Freeing all the memory allocated */
	      out:kfree(rtic_memdata1);
		return 0;
	case MXCTEST_RTIC_STATUS:
		rtic_stat = rtic_get_status();
     		printk("RTIC STATUS.0x%08lX\n", rtic_get_status());
		if((rtic_stat & 0x01)  == 0x01)
			return RTIC_SUCCESS; 
		else
			return RTIC_FAILURE;
	default:
		printk("MXC TEST IOCTL %d not supported\n", cmd);
		break;
	}
	return -EINVAL;
}

static int mxc_test_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations mxc_test_fops = {
	.owner = THIS_MODULE,
	.open = mxc_test_open,
	.release = mxc_test_release,
	.read = mxc_test_read,
	.write = mxc_test_write,
	.ioctl = mxc_test_ioctl,
};

static int __init mxc_test_init(void)
{
	struct class_device *temp_class;
	int res;

	res =
	    register_chrdev(MXC_TEST_MODULE_MAJOR, "mxc_test", &mxc_test_fops);

	if (res < 0) {
		printk(KERN_WARNING "MXC Test: unable to register the dev\n");
		return res;
	}

	mxc_test_class = class_create(THIS_MODULE, "mxc_test");
	if (IS_ERR(mxc_test_class)) {
		printk(KERN_ERR "Error creating mxc_test class.\n");
		unregister_chrdev(MXC_TEST_MODULE_MAJOR, "mxc_test");
		class_device_destroy(mxc_test_class,
				     MKDEV(MXC_TEST_MODULE_MAJOR, 0));
		return PTR_ERR(mxc_test_class);
	}

	temp_class = class_device_create(mxc_test_class, NULL,
					 MKDEV(MXC_TEST_MODULE_MAJOR, 0), NULL,
					 "mxc_test");
	if (IS_ERR(temp_class)) {
		printk(KERN_ERR "Error creating mxc_test class device.\n");
		class_device_destroy(mxc_test_class,
				     MKDEV(MXC_TEST_MODULE_MAJOR, 0));
		class_destroy(mxc_test_class);
		unregister_chrdev(MXC_TEST_MODULE_MAJOR, "mxc_test");
		return -1;
	}

	return 0;
}

static void __exit mxc_test_exit(void)
{
	unregister_chrdev(MXC_TEST_MODULE_MAJOR, "mxc_test");
	class_device_destroy(mxc_test_class, MKDEV(MXC_TEST_MODULE_MAJOR, 0));
	class_destroy(mxc_test_class);
}

module_init(mxc_test_init);
module_exit(mxc_test_exit);

MODULE_DESCRIPTION("Test Module for MXC drivers");
MODULE_LICENSE("GPL");


