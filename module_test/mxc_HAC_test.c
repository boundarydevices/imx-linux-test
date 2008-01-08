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

/*
 * General Include Files
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include "../include/mxc_test.h"
/*
 * Driver specific include files
 */
#include <asm/arch/mxc_security_api.h>

ulong *gtempu32ptr;
static struct class *mxc_test_class;

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

static int mxc_test_ioctl(struct inode *inode, struct file *file,
			  unsigned int cmd, unsigned long arg)
{
	ulong *tempu32ptr, tempu32, i = 0;

	ulong hac_length1 = 0x01, hac_length2 =
	    0x01, *hac_memdata1, *hac_memdata2;
	ulong hacc_phyaddr1, hacc_phyaddr2, rem1, rem2;
	hac_hash_rlt hash_res;
	ulong hac_config, hac_continue;

	tempu32 = (ulong) (*(ulong *) arg);
	tempu32ptr = (ulong *) arg;

	switch (cmd) {
	case MXCTEST_HAC_START_HASH:
		printk("HAC TEST DRV: Parameters received from user"
		       "space:\n");
		printk("HAC TEST DRV: Parameter1 0x%08lX\n", *(tempu32ptr + 0));
		printk("HAC TEST DRV: Parameter2 0x%08lX\n", *(tempu32ptr + 1));
		printk("HAC TEST DRV: Parameter3 0x%08lX\n", *(tempu32ptr + 2));
		printk("HAC TEST DRV: Parameter4 0x%08lX\n", *(tempu32ptr + 3));
		hac_length1 = *(tempu32ptr + 1);
		hac_length2 = *(tempu32ptr + 3);
		/* Allocating memory to copy user data to kernel memory */
		printk("HAC TEST DRV: Allocating memory for copying the "
		       "data to be hashed.\n");
		hac_memdata1 = (ulong *) kmalloc(sizeof(ulong) * ((hac_length1
								   + 1) * 0x10),
						 GFP_KERNEL);
		hac_memdata2 =
		    (ulong *) kmalloc(sizeof(ulong) *
				      ((hac_length2 + 1) * 0x10), GFP_KERNEL);
		HAC_DEBUG("HAC TEST DRV: Converting Virtual to Physical "
			  "Addr\n");
		hacc_phyaddr1 = virt_to_phys(hac_memdata1);

		hacc_phyaddr2 = virt_to_phys(hac_memdata2);
		printk("Before 64 byte alignment.\n");
		printk("Value of hac_memdata1 = 0x%08lX\n",
		       (ulong) hac_memdata1);
		printk("Value of hac_memdata2 = 0x%08lX\n",
		       (ulong) hac_memdata2);
		printk("Value of hacc_phyaddr1 = 0x%08lX\n", hacc_phyaddr1);
		printk("Value of hacc_phyaddr2 = 0x%08lX\n", hacc_phyaddr2);
		/* Physical address 64 byte alignment */
		if ((rem1 = ((ulong) hacc_phyaddr1 % 64)) != 0) {
			hacc_phyaddr1 = ((ulong) hacc_phyaddr1 + 64 - rem1);
		}
		if ((rem2 = ((ulong) hacc_phyaddr2 % 64)) != 0) {
			hacc_phyaddr2 = ((ulong) hacc_phyaddr2 + 64 - rem2);
		}
		/* Virtual address 64 byte alignement */
		hac_memdata1 = (ulong *) ((ulong) hac_memdata1 + 64 - rem1);

		hac_memdata2 = (ulong *) ((ulong) hac_memdata2 + 64 - rem2);

		printk("After 64 byte alignment.\n");
		printk("Value of hac_memdata1 = 0x%08lX\n",
		       (ulong) hac_memdata1);
		printk("Value of hac_memdata2 = 0x%08lX\n",
		       (ulong) hac_memdata2);
		printk("Value of hacc_phyaddr1 = 0x%08lX\n", hacc_phyaddr1);
		printk("Value of hacc_phyaddr2 = 0x%08lX\n", hacc_phyaddr2);
		/* 1 block of data is 512 bytes */
		printk("HAC TEST DRV: Copying data from user space.\n");
		i = copy_from_user(hac_memdata1, (ulong *) * tempu32ptr,
				   hac_length1 * 0x40);
		if (i != 0) {
			printk("HAC TEST DRV: Couldn't copy %lu bytes of"
			       "data from user space\n", i);
			return -EFAULT;
		}
		/* Copying 2nd memory block from user space to kernel space */
		/* 1 block of data is 512 bytes */
		i = copy_from_user(hac_memdata2, (ulong *) * (tempu32ptr + 2),
				   hac_length2 * 0x40);
		if (i != 0) {
			printk("HAC TEST DRV: Couldn't copy %lu bytes of"
			       "data from user space\n", i);
			return -EFAULT;
		}

		printk("HAC TEST DRV: Block lengths received from user:\n"
		       "Block Length 1: 0x%06lX\nBlock Length 2: 0x%06lX\n",
		       hac_length1, hac_length2);
		printk("HAC TEST DRV: Data received from user space:\n"
		       "Mem block 1: \n");
		tempu32 = hac_length1 * 0x10;
		for (i = 0; i < tempu32; i++) {
			printk("0x%08lX ", *(hac_memdata1 + i));
		}
		printk("\nMem block 2: \n");
		tempu32 = hac_length2 * 0x10;
		for (i = 0; i < tempu32; i++) {
			printk("0x%08lX ", *(hac_memdata2 + i));
		}

		HAC_DEBUG("HAC TEST DRV: Virtual address1 0x%08lX\n",
			  (ulong) hac_memdata1);
		HAC_DEBUG("HAC TEST DRV: Physical address1 0x%08lX\n",
			  hacc_phyaddr1);
		HAC_DEBUG("HAC TEST DRV: Virtual address2 0x%08lX\n",
			  (ulong) hac_memdata2);
		HAC_DEBUG("HAC TEST DRV: Physical address2 0x%08lX\n",
			  hacc_phyaddr2);
		/* Configures 16 Word Burst mode. */
		printk("HAC TEST DRV: 16 Word Burst Mode configure.\n");
		hac_burst_mode(HAC_16WORD_BURST);
		printk("HAC_TEST_DRV: 16 Word Burst read configure.\n");
		hac_burst_read(HAC_16WORD_BURST_READ);
		/* Configuring the HASH module */
		printk("\nHAC TEST DRV: Configuring the HASH module.\n");
		hac_config = hac_hash_data(hacc_phyaddr1, hac_length1,
					   HAC_START);
		if (hac_config == HAC_SUCCESS) {
			HAC_DEBUG("HACC TEST DRV: Configuring Hash Module is "
				  "success.\n");
		} else if (hac_config == HAC_FAILURE) {
			HAC_DEBUG("HACC TEST DRV: Configuring Hash Module is "
				  "failure.\n");
			goto hac_out;
		}
		printk("HAC TEST DRV: Waiting for the hashing to be "
		       "finished..\n");
		printk("Hac status while START 0x%08lX", hac_get_status());
		while (hac_hashing_status() == HAC_BUSY) {
			/* Do Nothing */
		}
		printk("A4 while 0x%08lX", hac_get_status());
		if (hac_hashing_status() == HAC_ERR) {
			printk("HAC TEST DRV: Encountered Error while"
			       "processing\n");
			return -EACCES;
		} else if (hac_hashing_status() == HAC_DONE) {
			printk("HAC TEST DRV: Hashing done. Continuing"
			       "hash.\n");
		} else if (hac_hashing_status() == HAC_UNKNOWN) {
			printk("HAC TEST DRV: Unknown HAC Status. HAC Unit"
			       "test Failed.\n");
			return -EACCES;
		}

		printk("HAC TEST DRV: Configuring for continuing hash.\n");
		/* Configuring the Hashing module to hash the remaining data */
		hac_continue = hac_hash_data(hacc_phyaddr2, hac_length2,
					     HAC_CONTINUE);
		if (hac_continue == HAC_SUCCESS) {
			HAC_DEBUG("HACC TEST DRV: Configuring Hash Module is "
				  "success.\n");
		} else if (hac_continue == HAC_FAILURE) {
			HAC_DEBUG("HACC TEST DRV: Configuring Hash Module is "
				  "failure.\n");
			goto hac_out;
		}
		printk("HAC TEST DRV: Waiting for the hashing to be "
		       "finished..\n");
		while (hac_hashing_status() == HAC_BUSY) {
			/* Do Nothing */
		}
		printk("Hac status while CONTINUE 0x%08lX", hac_get_status());
		if (hac_hashing_status() == HAC_ERR) {
			printk("HAC TEST DRV: Encountered Error while "
			       "processing\n");
			/* Bus error has occurred */
			/* Freeing all the memory allocated */
			goto hac_out;
		} else if (hac_hashing_status() == HAC_DONE) {
			printk("HAC TEST DRV: Hashing done. Padding "
			       "last block.\n");
		} else if (hac_hashing_status() == HAC_UNKNOWN) {
			printk("HAC TEST DRV: Unknown HAC Status. HAC Unit"
			       "test Failed.\n");
			return -EACCES;
		}
		printk("HAC TEST DRV: Last block padding.\n");
		/*
		 * Configuring the Hashing module to pad the hashed data block
		 * of data. The block length should now be equal to the total
		 * number of blocks hashed. The address field has no use as
		 * there is no hashing of data.
		 */
		hac_continue = hac_hash_data(hacc_phyaddr2, hac_length1 +
					     hac_length2, HAC_LAST);
		if (hac_continue == HAC_SUCCESS) {
			HAC_DEBUG("HACC TEST DRV: Hash padding module is "
				  "success.\n");
		} else if (hac_continue == HAC_FAILURE) {
			HAC_DEBUG("HACC TEST DRV: Padding the data is "
				  "failure.\n");
			goto hac_out;
		}
		printk("B4 while0x%08lX", hac_get_status());
		printk("HAC TEST DRV: Waiting for the padding to be "
		       "finished..\n");
		while (hac_hashing_status() == HAC_BUSY) {
			/* Do Nothing */
			printk("Inside HAC_BUSY0x%08lX", hac_get_status());
		}
		printk("Hac status while Last PAD0x%08lX", hac_get_status());
		if (hac_hashing_status() == HAC_ERR) {
			printk("HAC TEST DRV: Encountered Error while "
			       "processing\n");
			/* Bus error has occurred */
			/* Freeing all the memory allocated */
			goto hac_out;
		} else if (hac_hashing_status() == HAC_DONE) {
			printk("HAC TEST DRV: Padding done. Continuing "
			       "hash.\n");
		} else if (hac_hashing_status() == HAC_UNKNOWN) {
			printk("HAC TEST DRV: Unknown HAC Status. HAC Unit"
			       "test Failed.\n");
			return -EACCES;
		}
		/* Hashing done successfully. Now reading the hash data
		 * from memory.
		 */
		hac_hash_result(&hash_res);
		/* Displaying the hash data */
		for (i = 0; i < 5; i++) {
			printk("HAC TEST DRV: Hash result%lu : 0x%08lX\n",
			       i + 1, hash_res.hash_result[i]);
		}

		/* Freeing all the memory allocated */
	      hac_out:kfree(hac_memdata1);
		kfree(hac_memdata2);
		return 0;

	case MXCTEST_HAC_STATUS:
		printk("HAC TEST DRV: Status of the Hash module is: "
		       "0x%08lX", hac_get_status());
		return 0;
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
      owner:THIS_MODULE,
      open:mxc_test_open,
      release:mxc_test_release,
      read:mxc_test_read,
      write:mxc_test_write,
      ioctl:mxc_test_ioctl,
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
		class_device_destroy(mxc_test_class, MKDEV(MXC_TEST_MODULE_MAJOR, 0));
		return PTR_ERR(mxc_test_class);
	}

	temp_class = class_device_create(mxc_test_class, NULL,
					     MKDEV(MXC_TEST_MODULE_MAJOR, 0), NULL,
					     "mxc_test");
	if (IS_ERR(temp_class)) {
		printk(KERN_ERR "Error creating mxc_test class device.\n");
		class_device_destroy(mxc_test_class, MKDEV(MXC_TEST_MODULE_MAJOR, 0));
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
