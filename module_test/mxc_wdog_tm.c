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

static int mxc_wdog_tm_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t mxc_wdog_tm_read(struct file *file, char *buf, size_t count,
				loff_t * ppos)
{
	return 0;
}

static ssize_t mxc_wdog_tm_write(struct file *filp, const char *buf,
				 size_t count, loff_t * ppos)
{
	return 0;
}

#ifndef CONFIG_ARCH_MXC91221
extern volatile unsigned short g_wdog1_enabled, g_wdog2_enabled;
static unsigned short *g_wdog1_en = &g_wdog1_enabled;
#else
extern volatile unsigned short g_wdog_enabled;
static unsigned short *g_wdog1_en = &g_wdog_enabled;
#endif

extern void mxc_wd_init(int port);
static struct class *wdog_tm_class;

#if (CONFIG_ARCH_MXC91331 || CONFIG_ARCH_MXC91321 || CONFIG_ARCH_MXC91231)

/*!
 * This is the interrupt service routine for the watchdog timer.
 * It occurs only when a watchdog is enabled but not gets serviced in time.
 * This is a stub function as it just prints out a message now:
 *      WATCHDOG times out: <irq src numer>
 *
 * @param  irq          wdog interrupt source number
 * @param  dev_id       this parameter is not used
 * @param  regs         pointer to a structure containing the processor registers and state prior to servicing the
 *                      interrupt
 * @return always returns \b IRQ_HANDLED as defined in include/linux/interrupt.h.
 */
static irqreturn_t mxc_wdog_int(int irq, void *dev_id, struct pt_regs *regs)
{
	printk("\nWATCHDOG times out: %d\n", irq);
	return IRQ_HANDLED;
}

/*!
 * This function binds the watchdog isr to the 2nd WATCHDOG timer interrupt.
 */
static int mxc_wdog2_int_bind(void)
{
	int ret = 0;

	if (request_irq(INT_WDOG2, mxc_wdog_int, SA_INTERRUPT, "WDOG2", NULL)) {
		printk(KERN_ERR "%s: IRQ%d already in use.\n", "WDOG2",
		       INT_WDOG2);
		ret = -1;
	}
	return ret;
}
#endif

static void mxc_wdog_test(int *arg)
{
	spinlock_t wdog_lock = SPIN_LOCK_UNLOCKED;
	unsigned long wdog_flags, i = 0;
	int test_case = *arg;

	switch (test_case) {
	case 0:
		/* Normal opeation with both WDOGs enabled -
		 * System operates as normal. */
		*g_wdog1_en = 1;
		mxc_wd_init(0);
#if (CONFIG_ARCH_MXC91331 || CONFIG_ARCH_MXC91321 || CONFIG_ARCH_MXC91231)
		g_wdog2_enabled = 1;
		mxc_wd_init(1);
#endif
		break;
	case 1:
		/* Both WDOGs enabled but stuck in a loop with all
		 * interrupts disabled - System reboots. */
		*g_wdog1_en = 1;
		mxc_wd_init(0);
#if (CONFIG_ARCH_MXC91331 || CONFIG_ARCH_MXC91321 || CONFIG_ARCH_MXC91231)
		g_wdog2_enabled = 1;
		mxc_wd_init(1);
#endif

		spin_lock_irqsave(&wdog_lock, wdog_flags);
		while (1) {
			if ((i++ % 100000) == 0) {
				printk("1st watchdog test\n");
			}
		}
		spin_unlock_irqrestore(&wdog_lock, wdog_flags);
		break;
	case 2:
		/* The WDOG1 not serviced (both WDOGs are enabled) -
		 * System reboots. */

		*g_wdog1_en = 0;
		mxc_wd_init(0);
#if (CONFIG_ARCH_MXC91331 || CONFIG_ARCH_MXC91321 || CONFIG_ARCH_MXC91231)
		g_wdog2_enabled = 1;
		mxc_wd_init(1);
#endif
		break;

#if (CONFIG_ARCH_MXC91331 || CONFIG_ARCH_MXC91321 || CONFIG_ARCH_MXC91231)
	case 3:
		/* Only the WDOG2 enabled but not serviced -
		 * Stuck in the isr with "WATCHDOG times out: 55" messages displayed */
		g_wdog2_enabled = 0;
		mxc_wdog2_int_bind();
		mxc_wd_init(1);
		break;
	case 4:
		/* Both WDOGs enabled but the WDOG2 not serviced -
		 * First many "WATCHDOG times out: 55" messages displayed; Then
		 * the system will hang due to the 1st WDOG not being serviced. */
		*g_wdog1_en = 1;
		mxc_wd_init(0);
		mxc_wdog2_int_bind();
		g_wdog2_enabled = 0;
		mxc_wd_init(1);
		break;
#endif
	default:
		printk("invalide WDOG test case: %d\n", test_case);
		break;
	}
}

static int mxc_wdog_tm_ioctl(struct inode *inode, struct file *file,
			     unsigned int cmd, unsigned long arg)
{
	ulong *tempu32ptr, tempu32;

	tempu32 = (ulong) (*(ulong *) arg);
	tempu32ptr = (ulong *) arg;

	switch (cmd) {
	case MXCTEST_WDOG:
		mxc_wdog_test((int *)arg);
		return 0;
	default:
		printk("MXC TEST IOCTL %d not supported\n", cmd);
		break;
	}
	return -EINVAL;
}

static int mxc_wdog_tm_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations mxc_wdog_tm_fops = {
      owner:THIS_MODULE,
      open:mxc_wdog_tm_open,
      release:mxc_wdog_tm_release,
      read:mxc_wdog_tm_read,
      write:mxc_wdog_tm_write,
      ioctl:mxc_wdog_tm_ioctl,
};

static int __init mxc_wdog_tm_init(void)
{
	struct class_device *temp_class;
	int res;

	res =
	    register_chrdev(MXC_WDOG_TM_MAJOR, "mxc_wdog_tm",
			    &mxc_wdog_tm_fops);

	if (res < 0) {
		printk(KERN_WARNING "MXC Test: unable to register the dev\n");
		return res;
	}

	wdog_tm_class = class_create(THIS_MODULE, "mxc_wdog_tm");
	if (IS_ERR(wdog_tm_class)) {
		printk(KERN_ERR "Error creating mxc_wdog_tm class.\n");
		unregister_chrdev(MXC_WDOG_TM_MAJOR, "mxc_wdog_tm");
		class_device_destroy(wdog_tm_class,
				     MKDEV(MXC_WDOG_TM_MAJOR, 0));
		return PTR_ERR(wdog_tm_class);
	}

	temp_class = class_device_create(wdog_tm_class, NULL,
					 MKDEV(MXC_WDOG_TM_MAJOR, 0), NULL,
					 "mxc_wdog_tm");
	if (IS_ERR(temp_class)) {
		printk(KERN_ERR "Error creating mxc_wdog_tm class device.\n");
		class_device_destroy(wdog_tm_class,
				     MKDEV(MXC_WDOG_TM_MAJOR, 0));
		class_destroy(wdog_tm_class);
		unregister_chrdev(MXC_WDOG_TM_MAJOR, "mxc_wdog_tm");
		return -1;
	}

	return 0;
}

static void __exit mxc_wdog_tm_exit(void)
{
	unregister_chrdev(MXC_WDOG_TM_MAJOR, "mxc_wdog_tm");
	class_device_destroy(wdog_tm_class, MKDEV(MXC_WDOG_TM_MAJOR, 0));
	class_destroy(wdog_tm_class);
}

module_init(mxc_wdog_tm_init);
module_exit(mxc_wdog_tm_exit);

MODULE_DESCRIPTION("Test Module for MXC drivers");
MODULE_LICENSE("GPL");
