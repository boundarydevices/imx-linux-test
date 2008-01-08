/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file pmic_power_module.c
 * @brief This is the main file of Pmic Power driver.
 *
 * @ingroup PMIC_POWER
 */

/*
 * Includes
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/major.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/fs.h>

#include <asm/ioctl.h>
#include <asm/uaccess.h>
#include <asm/arch/pmic_status.h>
#include <asm/arch/pmic_power.h>
#include <asm/arch/pmic_external.h>

static int pmic_power_major;

static struct class *pmic_power_class;
#define CHECK_ERROR(a) \
do {                   \
	int ret = (a); \
	if(ret != PMIC_SUCCESS) {\
		printk(KERN_ERR "Test FAILED\n"); \
		return ret;     \
	}	       \
} while(0)

#ifdef CONFIG_MXC_PMIC_MC13783
int pmic_power_test_misc(void)
{
	int i, j;
	bool en1, en2, en3;
	bool sys_rst = 0;
	int deb_time = 0;
	t_pc_config pc_config, pc_config_1;

	printk(KERN_INFO
	       "Testing if Configuration of System Reset Buttons are OK\n");
	for (i = BT_ON1B; i <= BT_ON3B; i++) {
		for (j = 0; j < 4; j++) {
			CHECK_ERROR(pmic_power_set_conf_button(i, true, j));
			mdelay(1);
			CHECK_ERROR(pmic_power_get_conf_button
				    (i, &sys_rst, &deb_time));
			if (sys_rst != true || deb_time != j) {
				printk(KERN_ERR "Test FAILED\n");
				return PMIC_ERROR;
			}
		}
		CHECK_ERROR(pmic_power_set_conf_button(i, false, 0));
	}
	printk(KERN_INFO "Test PASSED\n");

	printk(KERN_INFO "Testing if Auto Reset is configured\n");
	en1 = 0;
	CHECK_ERROR(pmic_power_set_auto_reset_en(true));
	CHECK_ERROR(pmic_power_get_auto_reset_en(&en1));
	if (0 == en1) {
		printk(KERN_ERR "Test FAILED\n");
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_power_set_auto_reset_en(false));
	printk(KERN_INFO "Test PASSED\n");

	printk(KERN_INFO
	       "Testing if ESIM control voltage values are configured\n");
	en1 = en2 = en3 = 0;
	CHECK_ERROR(pmic_power_esim_v_en(true, true, true));
	CHECK_ERROR(pmic_power_gets_esim_v_state(&en1, &en2, &en3));
	if (0 == en1 || 0 == en2 || 0 == en3) {
		printk(KERN_ERR "Test FAILED\n");
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_power_esim_v_en(false, false, false));
	printk(KERN_INFO "Test PASSED\n");

	printk(KERN_INFO "Testing if Regen polarity is configured\n");
	en1 = 0;
	CHECK_ERROR(pmic_power_set_regen_inv(true));
	CHECK_ERROR(pmic_power_get_regen_inv(&en1));
	if (0 == en1) {
		printk(KERN_ERR "Test FAILED\n");
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_power_set_regen_inv(false));
	printk(KERN_INFO "Test PASSED\n");

	printk(KERN_INFO "Testing if battery detect enable is configured\n");
	en1 = 0;
	CHECK_ERROR(pmic_power_bat_det_en(true));
	CHECK_ERROR(pmic_power_get_bat_det_state(&en1));
	if (0 == en1) {
		printk(KERN_ERR "Test FAILED\n");
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_power_bat_det_en(false));
	printk(KERN_INFO "Test PASSED\n");

	printk(KERN_INFO "Testing if AUTO_VBKUP2 enable is configured\n");
	en1 = 0;
	CHECK_ERROR(pmic_power_vbkup2_auto_en(true));
	CHECK_ERROR(pmic_power_get_vbkup2_auto_state(&en1));
	if (0 == en1) {
		printk(KERN_ERR "Test FAILED\n");
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_power_vbkup2_auto_en(false));
	printk(KERN_INFO "Test PASSED\n");

	printk(KERN_INFO "Testing if Power Control Configurations are OK\n");
	pc_config.pc_enable = true;
	pc_config.pc_timer = 127;
	pc_config.pc_count_enable = true;
	pc_config.pc_count = 0;	/* read only */
	pc_config.pc_max_count = 15;
	pc_config.warm_enable = true;
	pc_config.user_off_pc = false;
	pc_config.clk_32k_user_off = false;
	pc_config.clk_32k_enable = true;
	pc_config.en_vbkup1 = true;
	pc_config.en_vbkup2 = true;
	pc_config.auto_en_vbkup1 = true;
	pc_config.auto_en_vbkup2 = true;
	pc_config.vhold_voltage = VH_2_5V;
	pc_config.mem_timer = 15;
	pc_config.mem_allon = true;

	CHECK_ERROR(pmic_power_set_pc_config(&pc_config));
	CHECK_ERROR(pmic_power_get_pc_config(&pc_config_1));

	if ((pc_config.pc_enable != pc_config_1.pc_enable)
	    || (pc_config.pc_timer != pc_config_1.pc_timer)
	    || (pc_config.pc_count_enable != pc_config_1.pc_count_enable)
	    || (pc_config.pc_count != pc_config_1.pc_count)
	    || (pc_config.pc_max_count != pc_config_1.pc_max_count)
	    || (pc_config.warm_enable != pc_config_1.warm_enable)
	    || (pc_config.clk_32k_enable != pc_config_1.clk_32k_enable)
	    || (pc_config.vhold_voltage != pc_config_1.vhold_voltage)
	    || (pc_config.mem_timer != pc_config_1.mem_timer)
	    || (pc_config.mem_allon != pc_config_1.mem_allon)) {
		printk(KERN_ERR "Test FAILED\n");
		return PMIC_ERROR;
	} else {
		printk(KERN_INFO "Test PASSED\n");
	}

	return PMIC_SUCCESS;
}
#elif CONFIG_MXC_PMIC_SC55112
int pmic_power_test_misc(void)
{
	return PMIC_SUCCESS;
}
#endif

/*!
 * This function implements IOCTL controls on a PMIC Power Control device
 * Driver.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int pmic_power_ioctl(struct inode *inode, struct file *file,
			    unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	t_regulator_cfg_param *cfg_param = NULL;

	if (_IOC_TYPE(cmd) != 'p')
		return -ENOTTY;

	switch (cmd) {
	case PMIC_REGULATOR_ON:
		ret = pmic_power_regulator_on((t_pmic_regulator) arg);
		break;

	case PMIC_REGULATOR_OFF:
		ret = pmic_power_regulator_off((t_pmic_regulator) arg);
		break;

	case PMIC_REGULATOR_SET_CONFIG:
		if ((cfg_param = kmalloc(sizeof(t_regulator_cfg_param),
					 GFP_KERNEL)) == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(cfg_param, (t_regulator_cfg_param *) arg,
				   sizeof(t_regulator_cfg_param))) {
			kfree(cfg_param);
			return -EFAULT;
		}

		ret =
		    pmic_power_regulator_set_config(cfg_param->regulator,
						    &cfg_param->cfg);

		kfree(cfg_param);
		break;

	case PMIC_REGULATOR_GET_CONFIG:
		if ((cfg_param = kmalloc(sizeof(t_regulator_cfg_param),
					 GFP_KERNEL)) == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(cfg_param, (t_regulator_cfg_param *) arg,
				   sizeof(t_regulator_cfg_param))) {
			kfree(cfg_param);
			return -EFAULT;
		}

		ret =
		    pmic_power_regulator_get_config(cfg_param->regulator,
						    &cfg_param->cfg);

		if (copy_to_user((t_regulator_cfg_param *) arg, cfg_param,
				 sizeof(t_regulator_cfg_param))) {
			kfree(cfg_param);
			return -EFAULT;
		}
		kfree(cfg_param);
		break;

	case PMIC_POWER_CHECK_MISC:
		ret = pmic_power_test_misc();
		break;

	default:
		return -EINVAL;
	}

	return ret;
}

/*!
 * This function implements the open method on a Pmic power device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_power_open(struct inode *inode, struct file *file)
{
	return 0;
}

/*!
 * This function implements the release method on a Pmic power device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_power_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations pmic_power_fops = {
	.owner = THIS_MODULE,
	.ioctl = pmic_power_ioctl,
	.open = pmic_power_open,
	.release = pmic_power_release,
};

/*
 * Init and Exit
 */

static int __init pmic_power_init(void)
{
	int ret = 0;
	pmic_version_t pmic_ver;
	struct class_device *temp_class;

	pmic_ver = pmic_get_version();
	if (pmic_ver.revision < 0) {
		printk(KERN_ERR "No PMIC device found\n");
		return -ENODEV;
	}

	pmic_power_major = register_chrdev(0, "pmic_power", &pmic_power_fops);
	if (pmic_power_major < 0) {
		printk(KERN_ERR "Unable to get a major for pmic_power");
		return -1;
	}

	pmic_power_class = class_create(THIS_MODULE, "pmic_power");
	if (IS_ERR(pmic_power_class)) {
		goto err1;
	}

	temp_class =
	    class_device_create(pmic_power_class, NULL,
				MKDEV(pmic_power_major, 0), NULL, "pmic_power");
	if (IS_ERR(temp_class)) {
		goto err2;
	}

	printk(KERN_INFO "Pmic Power loaded successfully\n");
	return ret;

      err2:
	class_destroy(pmic_power_class);
      err1:
	unregister_chrdev(pmic_power_major, "pmic_power");
	printk(KERN_ERR "Error creating pmic power class device.\n");
	return -1;
}

static void __exit pmic_power_exit(void)
{
	class_device_destroy(pmic_power_class, MKDEV(pmic_power_major, 0));
	class_destroy(pmic_power_class);
	unregister_chrdev(pmic_power_major, "pmic_power");
	printk(KERN_INFO "Pmic_power : successfully unloaded\n");
}

/*
 * Module entry points
 */

subsys_initcall(pmic_power_init);
module_exit(pmic_power_exit);

MODULE_DESCRIPTION("pmic_power driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
