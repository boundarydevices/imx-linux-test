/*
 * Copyright 2006-2008 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mman.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <asm/dma.h>
#include <asm/mach/dma.h>
#include <asm/delay.h>

#include <linux/device.h>

#include <linux/io.h>
#include <linux/delay.h>

#include <asm/arch/dma.h>

#include <asm/arch/hardware.h>

static int gMajor; //major number of device
static struct class *dma_tm_class;
static char *wbuf;
static char *rbuf;
static dma_addr_t wpaddr;
static dma_addr_t rpaddr;
static int sdma_channel = 0;
static int g_sand = 100;

#define SDMA_BUF_SIZE  1024
#define SDMA_SAND 13

static void mxc_printSDMAcontext(int channel)
{
	int i = 0, offset = 0;
	unsigned long tempAddr;
	volatile unsigned int result = 0, reg = 0;

	tempAddr = (unsigned long)(IO_ADDRESS(SDMA_BASE_ADDR));

	offset = 0x800 + (32 * channel);

	/* enable host ONCE */
	reg = __raw_readl(tempAddr + 0x40);
	reg |= 0x1;
	__raw_writel(reg, tempAddr + 0x40);

	/* enter debug mode */
	__raw_writel(0x05, tempAddr + 0x50);

	printk("Context of Channel %d\n", channel);
	for(i = 0; i < 32; i++) {
		mdelay(3);

		__raw_writel(((0x08 << 8) | ((offset >> 8) & 0xFF)) & 0xFFFF, tempAddr + 0x48);
		__raw_writel(0x02, tempAddr + 0x50);

		mdelay(3);
		__raw_writel(0x0011, tempAddr + 0x48);
		__raw_writel(0x02, tempAddr + 0x50);
		mdelay(3);

		/* 0x18xx xx = channel * 20 */
		reg = (((0x18 << 8) | (offset & 0xFF)) + i) & 0xFFFF;
		__raw_writel(reg, tempAddr + 0x48);
		__raw_writel(0x02, tempAddr + 0x50);
		mdelay(3);

		__raw_writel(0x5100, tempAddr + 0x48);
		__raw_writel(0x02, tempAddr + 0x50);
		mdelay(3);

		__raw_writel(0x01, tempAddr + 0x50);
		mdelay(3);

		result = __raw_readl(tempAddr + 0x44);
		/*
		 * Each data read into this register at address 0x53fd4044
		 * at this step is a register of the channel context
		 */
		printk("Context[%d] : %x\n", i, result);
	}
	/* exit debug mode */
	__raw_writel(0x03, tempAddr + 0x50);
}

static void sand(int rand)
{
	g_sand = rand;
	return;
}

static char rand(void)
{
	g_sand = g_sand * 12357 / 1103;
	return g_sand;
}

static void FillMem(char *p, int size, int rd)
{
	int i = 0;

	sand(rd);
	for(i = 0; i < size; i++) {
		*p++ = rand();
	}
        return;
}

static int Verify(char *p, int size, int rd)
{
	int i = 0,j = 0;
	char x;

	sand(rd);
	for(i = 0; i < size; i++) {
		x = rand();
		if (x != p[i]) {
			j++;
			break;
		}
	}

	if (j > 0) {
		return 1;
	} else {
		return 0;
	}
}

int sdma_open(struct inode * inode, struct file * filp)
{
	sdma_channel = mxc_dma_request(MXC_DMA_MEMORY, "SDMA Memory to Memory");
	if (sdma_channel < 0) {
		printk("Error opening the SDMA memory to memory channel\n");
		return -ENODEV;
	}

	wbuf = dma_alloc_coherent(NULL, SDMA_BUF_SIZE, &wpaddr, GFP_DMA);
	rbuf = dma_alloc_coherent(NULL, SDMA_BUF_SIZE, &rpaddr, GFP_DMA);

	return 0;
}

int sdma_release(struct inode * inode, struct file * filp)
{
	mxc_dma_free(sdma_channel);
	dma_free_coherent(NULL, SDMA_BUF_SIZE, wbuf, wpaddr);
	dma_free_coherent(NULL, SDMA_BUF_SIZE, rbuf, rpaddr);

	return 0;
}

ssize_t sdma_read (struct file *filp, char __user * buf, size_t count, loff_t * offset)
{
	if (Verify(rbuf, SDMA_BUF_SIZE, 100) == 0) {
		printk("SDMA memory test PASSED\n");
	} else {
		printk("SDMA memory test FAILED\n");
		mxc_printSDMAcontext(sdma_channel);
	}

	return 0;
}

ssize_t sdma_write(struct file * filp, const char __user * buf, size_t count, loff_t * offset)
{
	mxc_dma_requestbuf_t *sdmachnl_reqelem;

	if ((sdmachnl_reqelem = kmalloc(sizeof(mxc_dma_requestbuf_t), GFP_KERNEL)) == NULL) {
		return -ENOMEM;
	}

	FillMem(wbuf, SDMA_BUF_SIZE, 100);

	sdmachnl_reqelem->dst_addr = rpaddr;
	sdmachnl_reqelem->src_addr = wpaddr;
	sdmachnl_reqelem->num_of_bytes = SDMA_BUF_SIZE;

	mxc_dma_config(sdma_channel, sdmachnl_reqelem, 1,
		       MXC_DMA_MODE_READ);

	mxc_dma_enable(sdma_channel);

	kfree(sdmachnl_reqelem);

	printk("Data write completed\n");
	return 0;
}

struct file_operations dma_fops = {
	open:		sdma_open,
	release:	sdma_release,
	read:		sdma_read,
	write:		sdma_write,
};

int __init sdma_init_module(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
	struct device *temp_class;
#else
	struct class_device *temp_class;
#endif
	int error;

	/* register a character device */
	error = register_chrdev(0, "sdma_test", &dma_fops);
	if (error < 0) {
		printk("SDMA test driver can't get major number\n");
		return error;
	}
	gMajor = error;
	printk("SDMA test major number = %d\n",gMajor);

	dma_tm_class = class_create(THIS_MODULE, "sdma_test");
	if (IS_ERR(dma_tm_class)) {
		printk(KERN_ERR "Error creating sdma test module class.\n");
		unregister_chrdev(gMajor, "sdma_test");
		return PTR_ERR(dma_tm_class);
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
	temp_class = device_create(dma_tm_class, NULL,
				   MKDEV(gMajor, 0), "sdma_test");
#else
	temp_class = class_device_create(dma_tm_class, NULL,
					     MKDEV(gMajor, 0), NULL,
					     "sdma_test");
#endif
	if (IS_ERR(temp_class)) {
		printk(KERN_ERR "Error creating sdma test class device.\n");
		class_destroy(dma_tm_class);
		unregister_chrdev(gMajor, "sdma_test");
		return -1;
	}

	printk("SDMA test Driver Module loaded\n");
	return 0;
}

static void sdma_cleanup_module(void)
{
	unregister_chrdev(gMajor, "sdma_test");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
	device_destroy(dma_tm_class, MKDEV(gMajor, 0));
#else
	class_device_destroy(dma_tm_class, MKDEV(gMajor, 0));
#endif
	class_destroy(dma_tm_class);

	printk("SDMA test Driver Module Unloaded\n");
}


module_init(sdma_init_module);
module_exit(sdma_cleanup_module);

MODULE_AUTHOR("Freescale Semiconductor");
MODULE_DESCRIPTION("SDMA test driver");
MODULE_LICENSE("GPL");
