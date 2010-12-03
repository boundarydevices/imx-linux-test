/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/*
 * @file epdc_test_driver.c
 *
 * @brief Kernel Test module for MXC EPDC framebuffer driver
 *
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mman.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include <linux/device.h>
#include <linux/mxcfb.h>
#include <linux/mxcfb_epdc_kernel.h>

/* major number of device */
static int gMajor;
static struct class *epdc_tm_class;

static int epdc_test_open(struct inode * inode, struct file * filp)
{
	/* Open handle to actual EPDC FB driver */
	printk("Opening EPDC test handle\n");
	try_module_get(THIS_MODULE);

	return 0;
}

static int epdc_test_release(struct inode * inode, struct file * filp)
{
	/* Close handle to EPDC FB driver */
	printk("Closing EPDC test handle\n");
	module_put(THIS_MODULE);
	return 0;
}

static int epdc_test_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret = -EINVAL;

	switch (cmd) {
	case MXCFB_SET_WAVEFORM_MODES:
		{
			struct mxcfb_waveform_modes modes;
			if (!copy_from_user(&modes, argp, sizeof(modes))) {
				mxc_epdc_fb_set_waveform_modes(&modes, NULL);
				ret = 0;
			}
			break;
		}
	case MXCFB_SET_TEMPERATURE:
		{
			int temperature;
			if (!get_user(temperature, (int32_t __user *) arg))
				ret =
				    mxc_epdc_fb_set_temperature(temperature,
					NULL);
			break;
		}
	case MXCFB_SET_AUTO_UPDATE_MODE:
		{
			u32 auto_mode = 0;
			if (!get_user(auto_mode, (__u32 __user *) arg))
				ret =
				    mxc_epdc_fb_set_auto_update(auto_mode,
					NULL);
			break;
		}
	case MXCFB_SEND_UPDATE:
		{
			struct mxcfb_update_data upd_data;
			if (!copy_from_user(&upd_data, argp,
				sizeof(upd_data))) {
				ret = mxc_epdc_fb_send_update(&upd_data, NULL);
				if (ret == 0 && copy_to_user(argp, &upd_data,
					sizeof(upd_data)))
					ret = -EFAULT;
			} else {
				ret = -EFAULT;
			}

			break;
		}
	case MXCFB_WAIT_FOR_UPDATE_COMPLETE:
		{
			u32 update_marker = 0;
			if (!get_user(update_marker, (__u32 __user *) arg))
				ret =
				    mxc_epdc_fb_wait_update_complete(update_marker,
					NULL);
			break;
		}

	case MXCFB_SET_PWRDOWN_DELAY:
		{
			int delay = 0;
			if (!get_user(delay, (__u32 __user *) arg))
				ret =
				    mxc_epdc_fb_set_pwrdown_delay(delay, NULL);
			break;
		}

	case MXCFB_GET_PWRDOWN_DELAY:
		{
			int pwrdown_delay = mxc_epdc_get_pwrdown_delay(NULL);
			if (put_user(pwrdown_delay,
				(int __user *)argp))
				ret = -EFAULT;
			ret = 0;
			break;
		}
	default:
		printk("Invalid ioctl for epdc test driver.  ioctl = 0x%x\n",
			cmd);
		break;
	}

	return ret;
}

static struct file_operations epdc_test_fops = {
	open:		epdc_test_open,
	release:	epdc_test_release,
	unlocked_ioctl:	epdc_test_ioctl,
};

static int __init epdc_test_init_module(void)
{
	struct device *temp_class;
	int error;

	/* register a character device */
	error = register_chrdev(0, "epdc_test", &epdc_test_fops);
	if (error < 0) {
		printk("EPDC test driver can't get major number\n");
		return error;
	}
	gMajor = error;

	epdc_tm_class = class_create(THIS_MODULE, "epdc_test");
	if (IS_ERR(epdc_tm_class)) {
		printk(KERN_ERR "Error creating epdc test module class.\n");
		unregister_chrdev(gMajor, "epdc_test");
		return PTR_ERR(epdc_tm_class);
	}

	temp_class = device_create(epdc_tm_class, NULL,
				   MKDEV(gMajor, 0), NULL, "epdc_test");
	if (IS_ERR(temp_class)) {
		printk(KERN_ERR "Error creating epdc test class device.\n");
		class_destroy(epdc_tm_class);
		unregister_chrdev(gMajor, "epdc_test");
		return -1;
	}

	printk("EPDC test Driver Module loaded\n");
	return 0;
}

static void epdc_test_cleanup_module(void)
{
	unregister_chrdev(gMajor, "epdc_test");
	device_destroy(epdc_tm_class, MKDEV(gMajor, 0));
	class_destroy(epdc_tm_class);

	printk("EPDC test Driver Module Unloaded\n");
}


module_init(epdc_test_init_module);
module_exit(epdc_test_cleanup_module);

MODULE_AUTHOR("Freescale Semiconductor");
MODULE_DESCRIPTION("EPDC test driver");
MODULE_LICENSE("GPL");
