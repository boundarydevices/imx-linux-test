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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>

#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/semaphore.h>
#include <asm/uaccess.h>

#define BUF_MAX_SIZE		0x20

/*! 
 * SPI major number
 */
int major_spi;
static struct class *spi_class;

/*!
 * Global variable which indicates which SPI is used
 */
struct mxc_spi_lb {
	struct spi_device *spi;
	struct semaphore sem;
	char rbuf[BUF_MAX_SIZE];
	char wbuf[BUF_MAX_SIZE];
};
static struct mxc_spi_lb *lb_drv_data[3];

static int spi_rw(struct spi_device *spi, u8 * buf, size_t len)
{
	struct spi_transfer t = {
		.tx_buf = (const void *)buf,
		.rx_buf = buf,
		.len = len,
		.cs_change = 0,
		.delay_usecs = 0,
	};
	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	if (spi_sync(spi, &m) != 0 || m.status != 0)
		return -1;
	return m.actual_length;
}

/*!
 * This function implements the open method on a SPI device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int spi_loopback_open(struct inode *inode, struct file *file)
{
	unsigned int minor = MINOR(inode->i_rdev);

	file->private_data = (void *)(lb_drv_data[minor]);

	return 0;
}

/*!
 * This function implements the release method on a SPI device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int spi_loopback_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;

	return 0;
}

/*!
 * This function allows reading from a SPI device.
 *
 * @param        file        pointer on the file
 * @param        buf         pointer on the buffer
 * @param        count       size of the buffer
 * @param        off         offset in the buffer
  
 * @return       This function returns the number of read bytes.
 */
static ssize_t spi_loopback_read(struct file *file, char *buf,
				 size_t bytes, loff_t * off)
{
	struct mxc_spi_lb *priv = (struct mxc_spi_lb *)file->private_data;
	int res;

	if (bytes > BUF_MAX_SIZE) {
		bytes = BUF_MAX_SIZE;
	}

	down(&priv->sem);

	memset(priv->rbuf, 0, BUF_MAX_SIZE);
	memcpy(priv->rbuf, priv->wbuf, bytes);

	res = copy_to_user((void *)buf, (void *)priv->rbuf, bytes);
	if (res > 0) {
		up(&priv->sem);
		return -EFAULT;
	}
	up(&priv->sem);

	return bytes;
}

/*!
 * This function allows writing on a SPI device.
 *
 * @param        file        pointer on the file
 * @param        buf         pointer on the buffer
 * @param        count       size of the buffer
 * @param        off         offset in the buffer
 * @return       This function returns the number of written bytes.
 */
static ssize_t spi_loopback_write(struct file *file, const char *buf,
				  size_t count, loff_t * off)
{
	struct mxc_spi_lb *priv = (struct mxc_spi_lb *)file->private_data;
	int res = 0;

	if (count > BUF_MAX_SIZE) {
		count = BUF_MAX_SIZE;
	}
	down(&priv->sem);

	memset(priv->wbuf, 0, BUF_MAX_SIZE);
	res = copy_from_user((void *)priv->wbuf, (void *)buf, count);
	if (res > 0) {
		up(&priv->sem);
		return -EFAULT;
	}
	res = spi_rw(priv->spi, priv->wbuf, count);

	up(&priv->sem);

	return res;
}

/*!
 * This structure defines file operations for a SPI device.
 */
static struct file_operations spi_fops = {
	.owner = THIS_MODULE,
	.write = spi_loopback_write,
	.read = spi_loopback_read,
	.open = spi_loopback_open,
	.release = spi_loopback_release,
};

/*!
 * This function is called whenever the SPI loopback device is detected.
 *
 * @param	spi	the SPI loopback device
 */
static int __devinit spi_lb_probe(struct spi_device *spi)
{
	int master = 0;
	char dev_name[10];

	if (!strcmp(spi->dev.bus_id, "spi1.4")) {
		master = 1;
	} else if (!strcmp(spi->dev.bus_id, "spi2.4")) {
		master = 2;
	} else if (!strcmp(spi->dev.bus_id, "spi3.4")) {
		master = 3;
	}
	snprintf(dev_name, sizeof dev_name, "spi%d", master);

	master--;

	lb_drv_data[master] =
	    (struct mxc_spi_lb *)kmalloc(sizeof(struct mxc_spi_lb), GFP_KERNEL);
	if (lb_drv_data[master] == NULL) {
		return -ENOMEM;
	}
	if (IS_ERR(class_device_create(spi_class, NULL,
					   MKDEV(major_spi, master), NULL,
					   dev_name))) {
		kfree(lb_drv_data[master]);
		return -EFAULT;
	}

	lb_drv_data[master]->spi = spi;
	sema_init(&lb_drv_data[master]->sem, 1);

	spi->mode = SPI_MODE_2;
	spi->bits_per_word = 8;

	printk(KERN_INFO "Device %s probed\n", spi->dev.bus_id);

	return spi_setup(spi);
}

/*!
 * This function is called whenever the SPI loopback device is removed.
 *
 * @param	spi	the SPI loopback device
 */
static int __devexit spi_lb_remove(struct spi_device *spi)
{
	int master = 0;

	if (!strcmp(spi->dev.bus_id, "spi1.4")) {
		master = 1;
	} else if (!strcmp(spi->dev.bus_id, "spi2.4")) {
		master = 2;
	} else if (!strcmp(spi->dev.bus_id, "spi3.4")) {
		master = 3;
	}
	master--;

	kfree(lb_drv_data[master]);
	class_device_destroy(spi_class, MKDEV(major_spi, master));

	return 0;
}

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct spi_driver spi_lb_driver = {
	.driver = {
		   .name = "loopback_spi",
		   .bus = &spi_bus_type,
		   .owner = THIS_MODULE,
		   },
	.probe = spi_lb_probe,
	.remove = __devexit_p(spi_lb_remove),
	.suspend = NULL,
	.resume = NULL,
};

/*!
 * This function implements the init function of the SPI loopback device.
 * It is called when the module is loaded. It registers the SPI loopback
 * driver.
 */
static int __init spi_lb_init(void)
{
	printk("Registering the SPI Loopback Driver\n");
	major_spi = register_chrdev(0, "spi", &spi_fops);
	if (major_spi < 0) {
		printk(KERN_WARNING "Unable to get a major for loopback spi");
		return major_spi;
	}
	spi_class = class_create(THIS_MODULE, "spi");
	if (IS_ERR(spi_class)) {
		unregister_chrdev(major_spi, "spi");
	}

	return spi_register_driver(&spi_lb_driver);
}

/*!
 * This function implements the exit function of the SPI loopback device.
 * It is called when the module is unloaded. It unregisters the SPI loopback
 * driver.
 */
static void __exit spi_lb_exit(void)
{
	printk("Unregistering the SPI Loopback Driver\n");
	unregister_chrdev(major_spi, "spi");
	class_destroy(spi_class);
	spi_unregister_driver(&spi_lb_driver);
}

module_init(spi_lb_init);
module_exit(spi_lb_exit);

MODULE_DESCRIPTION("SPI loopback device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
