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
#include <linux/clk.h>
#include <asm/uaccess.h>
#include "../include/mxc_test.h"
/*
 * Driver specific include files
 */
#include <asm/arch/pmic_power.h>
#include <asm/arch/mxc_pm.h>
#include <asm/arch/gpio.h>
#include <linux/autoconf.h>

ulong *gtempu32ptr;
static struct class *mxc_test_class;

#if defined(CONFIG_ARCH_MXC91321) || defined(CONFIG_ARCH_MXC91331) || defined(CONFIG_ARCH_MXC91311)
#define CLK_OUT "cko2_clk"
#elif defined(CONFIG_ARCH_MX3)
#define CLK_OUT "cko1_clk"
#elif defined(CONFIG_MACH_MXC30031ADS)
#define CLK_OUT "ckoh_clk"
#else
#define CLK_OUT "cko_clk"
#endif

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

static int mxc_call_intscale(mxc_pm_test * arg)
{
	int result;

	printk("============== INT Scale IOCTL ==============\n");
	printk("Calling Integer Scaling \n");
	result = mxc_pm_dvfs(arg->armfreq, arg->ahbfreq, arg->ipfreq);
	printk("The result is %d \n", result);
	return 0;
}

static int mxc_call_pllscale(mxc_pm_test * arg)
{
#ifndef CONFIG_ARCH_MX3
	int result;

	printk("============== PLL Scale IOCTL ==============\n");
	printk("Calling PLL Scaling \n");
	result = mxc_pm_dvfs(arg->armfreq, arg->ahbfreq, arg->ipfreq);
	printk("The result is %d \n", result);
#endif
	return 0;
}

static int mxc_call_dvfs(mxc_pm_test * arg)
{
	int result;

	printk("============== INT/PLL Scale IOCTL ==============\n");
	result = mxc_pm_dvfs(arg->armfreq, arg->ahbfreq, arg->ipfreq);
	printk("The result is %d \n", result);
	return 0;
}

static int mxc_call_ckohsel(mxc_pm_test * arg)
{
	unsigned int rate;
	struct clk *cko_clk;
	struct clk *clk = NULL;

	cko_clk = clk_get(NULL, CLK_OUT);

	printk("============= CKOH SEL IOCTL ================\n");
	switch (arg->ckoh) {
	case CKOH_AP_SEL:
		clk = clk_get(NULL, "cpu_clk");
		break;
	case CKOH_AHB_SEL:
#if defined(CONFIG_MACH_MXC30031ADS)
		clk = clk_get(NULL, "ungated_ahb_clk");
#else
		clk = clk_get(NULL, "ahb_clk");
#endif
		break;
	case CKOH_IP_SEL:
		clk = clk_get(NULL, "ipg_clk");
		break;
	}
	clk_set_parent(cko_clk, clk);
	rate = clk_round_rate(cko_clk, 70000000);
	clk_set_rate(cko_clk, rate);
	clk_enable(cko_clk);
	printk("Clock output rate set to %u\n", rate);

	return 0;
}

#if defined(CONFIG_ARCH_MXC91321) || defined(CONFIG_ARCH_MXC91311) ||defined(CONFIG_ARCH_MXC91331) || defined(CONFIG_ARCH_MX3) || defined(CONFIG_ARCH_MX27)
static int mxc_call_lowpower(mxc_pm_test * arg)
{
	printk("============= Low-Power mode IOCTL ============\n");
	switch (arg->lpmd) {
	case 1:
		printk("To Test WAIT mode\n");
		mxc_pm_lowpower(WAIT_MODE);
		break;
	case 2:
		printk("To Test DOZE mode\n");
		mxc_pm_lowpower(DOZE_MODE);
		break;
	case 3:
		printk("To Test STOP mode\n");
		mxc_pm_lowpower(STOP_MODE);
		break;
	case 4:
		printk("To Test DSM mode\n");
		mxc_pm_lowpower(DSM_MODE);
		break;
	}
	return 0;
}
#else
static int mxc_call_lowpower(mxc_pm_test * arg)
{
	printk("============= Low-Power mode IOCTL ============\n");
	switch (arg->lpmd) {
	case 1:
		printk("To Test WAIT mode\n");
		mxc_pm_lowpower(WAIT_MODE);
		break;
	case 2:
		printk("To Test STOP mode\n");
		mxc_pm_lowpower(STOP_MODE);
		break;
	case 3:
		printk("To Test DSM mode\n");
		mxc_pm_lowpower(DSM_MODE);
		break;
	}
	return 0;
}
#endif

static int mxc_call_pmic_high(void)
{
#if !defined(CONFIG_ARCH_MXC91131)
	t_regulator_cfg_param cfgp;
	cfgp.regulator = SW_SW1A;
	pmic_power_regulator_get_config(cfgp.regulator, &cfgp.cfg);

	cfgp.cfg.voltage.sw1a = SW1A_1_6V;
	cfgp.cfg.voltage_lvs.sw1a = SW1A_1_6V;
	cfgp.cfg.voltage_stby.sw1a = SW1A_1V;
	cfgp.cfg.dvs_speed = DVS_4US;
	pmic_power_regulator_set_config(cfgp.regulator, &cfgp.cfg);
	printk("Configuring MC13783 for Hi (1.6V) voltage \n");
#endif
	return 0;
}

static int mxc_call_pmic_low(void)
{
#if !defined(CONFIG_ARCH_MXC91131)
	t_regulator_cfg_param cfgp;
	cfgp.regulator = SW_SW1A;
	pmic_power_regulator_get_config(cfgp.regulator, &cfgp.cfg);

	cfgp.cfg.voltage.sw1a = SW1A_1_2V;
	cfgp.cfg.voltage_lvs.sw1a = SW1A_1_2V;
	cfgp.cfg.voltage_stby.sw1a = SW1A_1V;
	cfgp.cfg.dvs_speed = DVS_4US;
	pmic_power_regulator_set_config(cfgp.regulator, &cfgp.cfg);
	printk("Configuring MC13783 for Lo (1.2V) voltage \n");
#endif
	return 0;
}

static int mxc_call_pmic_hilo(void)
{
#if !defined(CONFIG_ARCH_MXC91131)
	t_regulator_cfg_param cfgp;
	cfgp.regulator = SW_SW1A;
	pmic_power_regulator_get_config(cfgp.regulator, &cfgp.cfg);

#if defined(CONFIG_ARCH_MXC91321) || defined(CONFIG_ARCH_MXC91311)
	cfgp.cfg.voltage.sw1a = SW1A_1_2V;
	cfgp.cfg.voltage_lvs.sw1a = SW1A_1_6V;
#else
	cfgp.cfg.voltage.sw1a = SW1A_1_6V;
	cfgp.cfg.voltage_lvs.sw1a = SW1A_1_2V;
#endif
	cfgp.cfg.voltage_stby.sw1a = SW1A_1V;
	cfgp.cfg.dvs_speed = DVS_4US;
	cfgp.regulator = SW_SW1A;
	pmic_power_regulator_set_config(cfgp.regulator, &cfgp.cfg);
	printk("Configuring MC13783 for Lo (1.2V) and Hi (1.6V) voltage \n");
#endif
	return 0;
}

static int mxc_get_platform(unsigned long *platform)
{
if(cpu_is_mxc91231())
	{
	*platform=mxc91231;
	}
else if(cpu_is_mxc91221())
	{
	*platform=mxc91221;
	}
else if(cpu_is_mxc91321())
	{
	*platform=mxc91321;
	}
else if(cpu_is_mxc91311())
	{
	*platform=mxc91311;
	}
else if(cpu_is_mxc92323())
	{
	*platform=mxc92323;
	}
else if(cpu_is_mx21())
	{
	*platform=mx21;
	}
else if(cpu_is_mx27())
	{
	*platform=mx27;
	}
else if(cpu_is_mx31())
	{
	*platform=mx31;
	}
else if (cpu_is_mx32())
	{
	*platform=mx32;
	}
	return 0;
}

static int mxc_test_ioctl(struct inode *inode, struct file *file,
			  unsigned int cmd, unsigned long arg)
{
	ulong *tempu32ptr, tempu32;

	tempu32 = (ulong) (*(ulong *) arg);
	tempu32ptr = (ulong *) arg;

	switch (cmd) {
	case MXCTEST_PM_INTSCALE:
		return mxc_call_intscale((mxc_pm_test *) arg);
	case MXCTEST_PM_PLLSCALE:
		return mxc_call_pllscale((mxc_pm_test *) arg);

	case MXCTEST_PM_INT_OR_PLL:
		return mxc_call_dvfs((mxc_pm_test *) arg);

	case MXCTEST_PM_CKOH_SEL:
		return mxc_call_ckohsel((mxc_pm_test *) arg);

	case MXCTEST_PM_LOWPOWER:
		return mxc_call_lowpower((mxc_pm_test *) arg);

	case MXCTEST_PM_PMIC_HIGH:
		return mxc_call_pmic_high();
	case MXCTEST_PM_PMIC_LOW:
		return mxc_call_pmic_low();
	case MXCTEST_PM_PMIC_HILO:
		return mxc_call_pmic_hilo();
	case MXCTEST_GET_PLATFORM:
		return mxc_get_platform((ulong *)arg);
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
		class_device_destroy(mxc_test_class,
				     MKDEV(MXC_TEST_MODULE_MAJOR, 0));
		return PTR_ERR(mxc_test_class);
	}

	temp_class = class_device_create(mxc_test_class, NULL,
					 MKDEV(MXC_TEST_MODULE_MAJOR, 0),
					 NULL, "mxc_test");
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
