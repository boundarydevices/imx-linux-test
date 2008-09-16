/*
 * Memalloc, encoder memory allocation driver (kernel module)
 *
 * Copyright (C) 2005  Hantro Products Oy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>		/* needed for __init,__exit directives */
#include <linux/mm.h>		/* remap_page_range / remap_pfn_range */
#include <linux/slab.h>		/* kmalloc */
#include <linux/fs.h>		/* for struct file_operations */
#include <linux/errno.h>	/* standard error codes */

#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/dma-mapping.h>

#include "memalloc.h"		/* memalloc specific */

/* and this is our MAJOR; use 0 for dynamic allocation (recommended)*/
static s32 memalloc_major;
static struct class *memalloc_class;
static s32 open_count;

/* here's all the must remember stuff */
struct allocation {
	struct list_head list;
	u32 phys_addr;
	u32 cpu_addr;
	u32 size;
};

static DEFINE_SPINLOCK(mem_lock);
static LIST_HEAD(heap_list);

static struct allocation *FindEntryByBus(u32 phys_addr);
static s32 AllocMemory(u32 * phys_addr, u32 size);
static s32 FreeMemory(u32 phys_addr);
static s32 MapBuffer(struct file *filp, struct vm_area_struct *vma);

/*!
 * This function frees all the buffers allocated by this module.
 */
static void memalloc_free_buffers(void)
{
	struct list_head *ptr;
	struct allocation *entry;

	while (!list_empty(&heap_list)) {
		ptr = heap_list.next;
		entry = list_entry(ptr, struct allocation, list);

		pr_debug("memalloc freeall: phys = %x, virt = %x size = %d\n",
			 entry->phys_addr, entry->cpu_addr, entry->size);

		dma_free_coherent(NULL, entry->size, (void *)
				  entry->cpu_addr, entry->phys_addr);
		list_del(ptr);
		kfree(entry);
	}

	pr_debug("memalloc freed all buffers\n");
}

/*!
 * The device's mmap method. The VFS has kindly prepared the process's
 * vm_area_struct for us, so we examine this to see what was requested.
 *
 * @param   filp	pointer to struct file
 * @param   vma		pointer to struct vma_area_struct
 *
 * @return  This function returns 0 if successful or -ve value on error.
 *
 */
static s32 memalloc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	s32 result;

	spin_lock(&mem_lock);
	result = MapBuffer(filp, vma);
	spin_unlock(&mem_lock);

	return result;
}

/*!
 * This funtion is called to handle ioctls.
 *
 * @param   inode	pointer to struct inode
 * @param   filp	pointer to struct file
 * @param   cmd		ioctl command
 * @param   arg		user data
 *
 * @return  This function returns 0 if successful or -ve value on error.
 */
static s32 memalloc_ioctl(struct inode *inode, struct file *filp,
			  u32 cmd, ulong arg)
{
	s32 err = 0, retval = 0;
	u32 phys_addr = 0, size = 0;

	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != MEMALLOC_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > MEMALLOC_IOC_MAXNR)
		return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	switch (cmd) {
	case MEMALLOC_IOCHARDRESET:
		memalloc_free_buffers();
		break;

	case MEMALLOC_IOCXGETBUFFER:
		__get_user(size, (u32 *) arg);
		retval = AllocMemory(&phys_addr, size);
		__put_user(phys_addr, (u32 *) arg);
		break;

	case MEMALLOC_IOCSFREEBUFFER:
		__get_user(phys_addr, (u32 *) arg);
		retval = FreeMemory(phys_addr);
		break;

	default:
		retval = -EINVAL;
	}

	return retval;
}

/*!
 * This funtion is called when the device is opened.
 *
 * @param   inode	pointer to struct inode
 * @param   filp	pointer to struct file
 *
 * @return  This function returns 0.
 *
 */
static s32 memalloc_open(struct inode *inode, struct file *filp)
{
	open_count++;
	pr_debug("memalloc device opened\n");
	return 0;
}

/*!
 * This funtion is called when the device is closed.
 *
 * @param   inode	pointer to struct inode
 * @param   filp	pointer to struct file
 *
 * @return  This function returns 0.
 *
 */
static s32 memalloc_release(struct inode *inode, struct file *filp)
{
	if (--open_count == 0) {
		/* Free the buffers */
		memalloc_free_buffers();
	}

	pr_debug("memalloc device closed\n");
	return 0;
}

/* VFS methods */
static struct file_operations memalloc_fops = {
      mmap:memalloc_mmap,
      open:memalloc_open,
      release:memalloc_release,
      ioctl:memalloc_ioctl,
};

/*!
 * This funtion allocates physical contigous memory.
 *
 * @param   phys_addr 	physical address
 * @param   size 	size of memory to be allocated
 *
 * @return  This function returns 0 if successful or -ve value on error.
 *
 */
static s32 AllocMemory(u32 * phys_addr, u32 size)
{
	struct allocation *entry;

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (entry == 0) {
		pr_debug("memalloc: failed to alloc entry\n");
		return -ENOMEM;
	}

	/* alloc memory */
	size = PAGE_ALIGN(size);
	entry->cpu_addr =
	    (u32) dma_alloc_coherent(NULL, size,
				     (dma_addr_t *) & entry->phys_addr,
				     GFP_DMA | GFP_KERNEL);

	if (entry->cpu_addr == 0) {
		pr_debug("memalloc: failed to alloc memory\n");
		*phys_addr = 0;
		return -ENOMEM;
	}

	*phys_addr = entry->phys_addr;
	entry->size = size;

	pr_debug("memalloc alloc: phys = %x, virt = %x size = %d\n",
		 entry->phys_addr, entry->cpu_addr, size);

	list_add_tail(&entry->list, &heap_list);
	return 0;
}

/*!
 * This funtion frees the DMAed memory.
 *
 * @param phys_addr	memory to be freed
 *
 * @return  This function returns 0 if successful or -ve value on error.
 */
static s32 FreeMemory(u32 phys_addr)
{
	struct allocation *entry = NULL;

	entry = FindEntryByBus(phys_addr);
	if (entry == NULL) {
		return -EINVAL;
	}

	pr_debug("memalloc free: phys = %x, virt = %x size = %d\n",
		 entry->phys_addr, entry->cpu_addr, entry->size);

	/* and now free the memory */
	list_del(&entry->list);

	dma_free_coherent(NULL, entry->size, (void *)entry->cpu_addr,
			  entry->phys_addr);
	kfree(entry);
	return 0;
}

/*!
 * This function maps the shared buffer in memory
 *
 * @param   filp 	pointer to struct file
 * @param   vma		pointer to struct vm_area_struct
 *
 * @return  This function returns 0 if successful or -ve value on error.
 */
static s32 MapBuffer(struct file *filp, struct vm_area_struct *vma)
{
	struct allocation *entry;
	u32 offset = vma->vm_pgoff << PAGE_SHIFT;
	u32 start = (u32) vma->vm_start;
	u32 size = (u32) (vma->vm_end - vma->vm_start);
	u32 phys;

	entry = FindEntryByBus(offset);
	if (entry == NULL) {
		pr_debug("mmap failed, map buffer entry not found in list\n");
		return -EINVAL;
	}

	/* if userspace tries to mmap beyond end of our buffer, fail */
	if (size != entry->size) {
		pr_debug("mmap failed, buffer size does not match\n");
		return -EINVAL;
	}

	vma->vm_flags |= VM_RESERVED;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	phys = entry->phys_addr;

	if (remap_pfn_range(vma, start, phys >> PAGE_SHIFT, size,
			    vma->vm_page_prot)) {
		pr_debug("mmap failed\n");
		return -EAGAIN;
	}

	return 0;
}

/*!
 * This function searches the allocation link list to find the entry
 * corresponding to physical address.
 *
 * @param  phys_address 	The address that needs to be searched
 *
 * @return  The function returns the entry if successful else NULL.
 */
struct allocation *FindEntryByBus(u32 phys_addr)
{
	struct list_head *ptr;
	struct allocation *entry;

	list_for_each(ptr, &heap_list) {
		entry = list_entry(ptr, struct allocation, list);
		if (entry->phys_addr == phys_addr) {
			return entry;
		}
	}

	return NULL;
}

/*!
 * This function is called when the module is loaded. It creates the
 * device entry.
 *
 * @return  The function returns 0 if successful.
 */
s32 __init memalloc_init(void)
{
	s32 result;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
	struct device *temp_class;
#else
	struct class_device *temp_class;
#endif

	result = register_chrdev(memalloc_major, "memalloc", &memalloc_fops);
	if (result <= 0) {
		pr_debug("memalloc: unable to get major\n");
		return -EIO;
	}

	memalloc_major = result;

	memalloc_class = class_create(THIS_MODULE, "memalloc");
	if (IS_ERR(memalloc_class)) {
		pr_debug("memalloc: error creating memalloc class\n");
		goto error1;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
	temp_class = device_create(memalloc_class, NULL,
				   MKDEV(memalloc_major, 0), "memalloc");
#else
	temp_class = class_device_create(memalloc_class, NULL,
					 MKDEV(memalloc_major, 0), NULL,
					 "memalloc");
#endif
	if (IS_ERR(temp_class)) {
		pr_debug("memalloc: Error creating class device\n");
		goto error2;
	}

	printk(KERN_INFO "memalloc: module inserted\n");
	return 0;

      error2:
	class_destroy(memalloc_class);
      error1:
	unregister_chrdev(memalloc_major, "memalloc");
	printk(KERN_INFO "memalloc: Error! \n");
	return -EIO;
}

/*!
 * This function is called when the module is unloaded. It deletes the
 * device entry and frees the memory.
 *
 * @return  The function returns 0 if successful.
 */
void __exit memalloc_cleanup(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
	device_destroy(memalloc_class, MKDEV(memalloc_major, 0));
#else
	class_device_destroy(memalloc_class, MKDEV(memalloc_major, 0));
#endif
	class_destroy(memalloc_class);
	unregister_chrdev(memalloc_major, "memalloc");
	memalloc_free_buffers();
	printk(KERN_INFO "memalloc: module removed\n");
}

module_init(memalloc_init);
module_exit(memalloc_cleanup);

/* module description */
MODULE_AUTHOR("Hantro Products Oy");
MODULE_DESCRIPTION("Linear memory allocation for h/w MPEG4 encoder");
MODULE_SUPPORTED_DEVICE("5251/4251 MPEG4 Encoder");
MODULE_LICENSE("GPL");
