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

/* +FHDR-----------------------------------------------------------------------
 * ----------------------------------------------------------------------------
 * FILE NAME      : scc_test_driver.h
 * DEPARTMENT     : Security Technology Center (STC), NCSG
 * AUTHOR         : Ron Harvey (r66892)
 * ----------------------------------------------------------------------------
 * REVIEW(S) :
 * ----------------------------------------------------------------------------
 * RELEASE HISTORY
 * VERSION DATE       AUTHOR       DESCRIPTION
 * 0.0.1   2005-02-04 R. Harvey    Initial version
 * 0.0.8   2005-03-08 R. Harvey    Implementation complete
 * ----------------------------------------------------------------------------
 * KEYWORDS : SCC, SMN, SCM, Security, Linux driver
 * ----------------------------------------------------------------------------
 * PURPOSE: Provide a test driver for access to the SCC (Security Controller)
 * ----------------------------------------------------------------------------
 * REUSE ISSUES
 * Version 2 of this block will look a lot different
 * -FHDR-----------------------------------------------------------------------
 */

/**
 * @file scc_test_driver.c
 * @brief This is a test driver for the SCC.
 *
 * This driver and its associated reference test program are intended
 * to demonstrate the use of the SCC block in various ways.
 *
 * The driver runs in a Linux environment.  Portation notes are
 * included at various points which should be helpful in moving the
 * code to a different environment.
 *
 */

#include <linux/device.h>
#include "../include/scc_test_driver.h"

/*
 * Definitions to make this a Linux Driver Module
 */

/** Tell Linux where to invoke driver on module load  */
module_init(scc_test_init);
/** Tell Linux where to invoke driver on module unload  */
module_exit(scc_test_cleanup);

/** Tell Linux this isn't GPL code */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Freescale Semiconductor");
MODULE_DESCRIPTION("Test Device Driver for SCC (SMN/SCM) Driver");


/** Allow user to configure major node value at insmod */
//MODULE_PARM(scc_test_major_node, "i");
MODULE_PARM_DESC(scc_test_major_node, "Device Major Node number");

/** look into module_param()  in 2.6*/

/** Create a place to track/notify sleeping processes */
DECLARE_WAIT_QUEUE_HEAD(waitQueue);

/** The /dev/node value for user-kernel interaction */
int scc_test_major_node = SCC_TEST_MAJOR_NODE;

static struct class *scc_tm_class;


/** saved-off pointer of configuration information */
scc_config_t* scc_cfg;

/**
 * Interface jump vector for calls into the device driver.
 *
 * This struct changes frequently in Linux kernel versions.  By initializing
 * elements by name, we can avoid structure mismatches.  Other elements get
 * NULL/0 by default (after all, this is a global initializer).
 */
static struct file_operations scc_test_fops = {
    .owner = THIS_MODULE,
    .open = scc_test_open,
    .ioctl = scc_test_ioctl,
    .mmap = scc_test_mmap,
    .release = scc_test_release
};


/***********************************************************************
 * scc_test_init()                                                     *
 **********************************************************************/
/**
 * Driver interface function which initializes the Linux device driver.
 *
 * It registers the SCC character device, sets up the base address for
 * the SCC, and registers interrupt handlers.  It is called during
 * insmod(8).  The interface is defined by the Linux DDI/DKI.
 *
 * @return This function returns 0 upon success or a (negative) errno
 * code on failure.
 *
 * @todo An appropriate "major" number must be selected, or generated
 * automatically (along with mknod() call!?)
 */
static int scc_test_init (void)
{
    int      error_code = 0;
    uint32_t smn_status;

    /* call the real driver here */
    scc_cfg = scc_get_configuration();

    /* call the real driver here */
    if (! (error_code = scc_read_register(SMN_STATUS, &smn_status))) {

        /* set up driver as character driver */
        if (! (error_code = setup_user_driver_interaction())) {

            /* Determine status of SCC */
            if ((smn_status & SMN_STATUS_STATE_MASK) == SMN_STATE_FAIL) {
                printk("SCC TEST Driver: SCC is in Fail mode\n");
            }
            else {
                printk("SCC TEST Driver: Ready\n");
            }
        }
        else {
        }
    }
    else {
        printk("SCC TEST: Could not read SMN_STATUS register: %d\n",
               error_code);
    }

    scc_monitor_security_failure(scc_test_report_failure);

    /* any errors, we must undo what we've done */
    if (error_code) {
        scc_test_cleanup();
    }

    return error_code;
}



/***********************************************************************
 * scc_read_open()                                                     *
 **********************************************************************/
/**
 * Driver interface function for open() system call.
 *
 * This function increments the "IN USE" count.  It is called during
 * an open().  The interface is defined by the Linux DDI/DKI.
 *
 * @param inode struct that contains the major and minor numbers for
 * the device.
 * @param file File pointer.
 *
 * @return 0 if successful, error code if not
 */
static int
scc_test_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    try_module_get(THIS_MODULE);
#else
    MOD_INC_USE_COUNT;
#endif

#ifdef SCC_DEBUG
    printk ("SCC TEST: Device Opened\n");
#endif
    return 0;
}


/***********************************************************************
 * scc_test_release()                                                  *
 **********************************************************************/
/**
 * Driver interface function for close() system call.
 *
 * This function decrements the "IN USE" count.  It is called during a
 * close().  The interface is defined by the Linux DDI/DKI.
 *
 * @param inode struct that contains the major and minor numbers.
 * @param file file pointer
 *
 * @return 0 (always - errors are ignored)
 */
static int
scc_test_release(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    module_put(THIS_MODULE);
#else
    MOD_DEC_USE_COUNT;
#endif

#ifdef SCC_DEBUG
    printk ("SCC TEST: Inside release\n");
#endif
    return 0;
}


/***********************************************************************
 * scc_test_cleanup()                                                  *
 **********************************************************************/
/**
 * Driver interface function for unloading the device driver.
 *
 * This function is called during rmmod(8).  The interface is defined
 * by the Linux DDI/DKI.
 *
 * It deregisters the SCC character device, unmaps memory, and
 * deregisters the interrupt handler(s).
 *
 * Called by the kernel during an @c rmmod(8) operation, but also
 * during error handling from #scc_test_init().
 *
 */
static void
scc_test_cleanup (void)
{
    /* turn off the mapping to the device special file */
    if (scc_test_major_node) {
	class_device_destroy(scc_tm_class, MKDEV(scc_test_major_node, 0));
	class_destroy(scc_tm_class);
	unregister_chrdev(scc_test_major_node, SCC_TEST_DRIVER_NAME);
        scc_test_major_node = 0;
    }

#ifdef SCC_DEBUG
    printk ("SCC TEST: Cleaned up\n");
#endif

    return;
}


/***********************************************************************
 * scc_test_ioctl()                                                    *
 **********************************************************************/
/**
 * Driver interface function for ioctl() system call.
 *
 * This function serves as a control for the commands being
 * passed by the application code.  Depending on what command has been
 * sent, a specific function will occur.  The interface is defined by
 * the Linux DDI/DKI.
 *
 * This routine handles the following valid commands:
 *
 * - #SCC_GET_CONFIGURATION - Return driver, SMN, and SCM versions, memory
      sizes, block size.
 * - #SCC_READ_REG - Read register from SCC.
 * - #SCC_WRITE_REG - Write register to SCC.
 * - #SCC_ENCRYPT - Encrypt Red memory into Black memory.
 * - #SCC_DECRYPT - Decrypt Black memory into Red memory.
 *
 * @pre Application code supplies a command with the related data (via the
 * scc_data struct)
 *
 * @post A specific action is performed based on the requested command.
 *
 * @param inode struct that contains the major and minor numbers.
 * @param file File pointer.
 * @param cmd The requested command supplied by application code.
 * @param scc_data Input struct provided by application code.
 *
 * @return 0 or an error code (IOCTL_SCC_xxx)
 */
static int
scc_test_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
               unsigned long scc_data)
{
    int   error_code = IOCTL_SCC_OK;

    switch (cmd) {
    case SCC_TEST_GET_CONFIGURATION:
        error_code = scc_test_get_configuration(scc_data);
        break;

    case SCC_TEST_READ_REG:
        error_code = scc_test_read_register(scc_data);
        break;

    case SCC_TEST_WRITE_REG:
        error_code = scc_test_write_register(scc_data);
        break;

    case SCC_TEST_ENCRYPT:
    case SCC_TEST_DECRYPT:
        /* kick off the function */
        error_code = scc_test_cipher(cmd, scc_data);
        break;              /* encrypt/decrypt */

    case SCC_TEST_ALLOC_SLOT:
        error_code = scc_test_alloc_slot(cmd, scc_data);
        break;

    case SCC_TEST_DEALLOC_SLOT:
        error_code = scc_test_dealloc_slot(cmd, scc_data);
        break;

    case SCC_TEST_LOAD_SLOT:
        error_code = scc_test_load_slot(cmd, scc_data);
        break;

    case SCC_TEST_ENCRYPT_SLOT:
        error_code = scc_test_encrypt_slot(cmd, scc_data);
        break;

    case SCC_TEST_GET_SLOT_INFO:
        error_code = scc_test_get_slot_info(cmd, scc_data);
        break;

    case SCC_TEST_SET_ALARM:
        scc_set_sw_alarm();     /* call the real driver here */
        break;

    case SCC_TEST_ZEROIZE:
        error_code = scc_test_zeroize(scc_data);
        break;

    default:
#ifdef SCC_DEBUG
        printk("SCC TEST: Error in ioctl(): (0x%x) is an invalid "
               "command (0x%x,0x%x)\n", cmd, SCC_TEST_GET_CONFIGURATION,
               SCC_TEST_ALLOC_SLOT);
        printk("uint64 is %d, alloc is %d\n", sizeof(uint64_t),
               sizeof(scc_alloc_slot_access));
#endif
        error_code = -IOCTL_SCC_INVALID_CMD;
        break;

    } /* End switch */

    return error_code;
}


/***********************************************************************
 * scc_test_mmap()                                                     *
 **********************************************************************/
/**
 * Map the SMN's register interface into user space for user access to
 * #SMN_SEQUENCE_CHECK, #SMN_BIT_BANK_DECREMENT, #SMN_PLAINTEXT_CHECK,
 * and #SMN_CIPHERTEXT_CHECK.
 *
 * This is some test code for a) allowing user access to ASC/AIC features,
 * and b) experiments for how SCC2 would work.
 */
static int
scc_test_mmap(struct file *filep, struct vm_area_struct *vma)
{

#if 0
    /*
     * Disable this system call.  This feature is not available in the driver.
     */
    return -EFAULT;
#else
    /* This version is allowing the User App to touch the SCC, but the SMN
     * is turning on its CACHEABLE_ACCESS and USER_ACCESS bits and giving
     * zero on reads (and killing processes which write??)
     */
    printk("Mapping SCC at %p for user\n", (void*)SCC_BASE_ADDR);
#if 1
    vma->vm_pgoff = SCC_BASE_ADDR >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;
    return remap_pfn_range(vma, vma->vm_start,
                           SCC_BASE_ADDR >> PAGE_SHIFT,
                           8192, vma->vm_page_prot);
#elif 0
    return io_remap_page_range(vma, vma->vm_start,
                               SCC_BASE_ADDR,
                               8192,
                               vma->vm_page_prot);
#endif

#endif
}


/***********************************************************************
 * scc_test_get_configuration()                                        *
 **********************************************************************/
/**
 * Internal routine to handle ioctl command #SCC_GET_CONFIGURATION.
 *
 * @param scc_data Address is user space of scc_configuration_access
 * which is passed with the ioctl() system call.
 *
 * @return 0 on success, IOCTL_xxx on failure.
 *
 * This function does not access the SCC, it just passes up static,
 * previously retrieved information.
 *
 */
static int
scc_test_get_configuration(unsigned long scc_data)
{
    int error_code = IOCTL_SCC_OK;

#ifdef SCC_DEBUG
    printk("SCC TEST: Configuration\n");
#endif

    /* now copy out (write) the data into user space */
    /** @todo make sure scc_get_configuration never returns null! */
    if (copy_to_user((void *)scc_data, scc_get_configuration(),
                     sizeof(scc_config_t))) {
#ifdef SCC_DEBUG
        printk("SCC TEST: Error writing data to user\n");
#endif
        error_code = -IOCTL_SCC_IMPROPER_ADDR;
    }

    return error_code;

} /* scc_get_configuration */


/***********************************************************************
 * scc_read_register()                                                 *
 **********************************************************************/
/**
 * Read a register value from the SCC.
 *
 * @param scc_data The address in user memory of the scc_reg_access struct
 * passed in the ioctl().
 *
 * @return 0 for success, an error code on failure.
 */
static int
scc_test_read_register(unsigned long scc_data)
{
    scc_reg_access reg_struct;
    scc_return_t   scc_return = -1;
    int            error_code = IOCTL_SCC_OK;

    if (copy_from_user(&reg_struct, (void *)scc_data, sizeof(reg_struct))) {
#ifdef SCC_DEBUG
        printk("SCC TEST: Error reading reg struct from user\n");
#endif
        error_code = -IOCTL_SCC_IMPROPER_ADDR;
    }

    else {
        /* call the real driver here */
        scc_return = scc_read_register(reg_struct.reg_offset,
                                       &reg_struct.reg_data);
        if (scc_return != SCC_RET_OK) {
            error_code = -IOCTL_SCC_IMPROPER_ADDR;
        }
    }

    reg_struct.function_return_code = scc_return;
    copy_to_user((void *)scc_data, &reg_struct, sizeof(reg_struct));
    return error_code;
}


/***********************************************************************
 * scc_write_register()
 **********************************************************************/
/**
 * Write a register value to the SCC.
 *
 * @param scc_data The address in user memory of the scc_reg_access struct
 * passed in the ioctl().
 *
 * @return 0 for success, an error code on failure.
 */
static int
scc_test_write_register(unsigned long scc_data) {
    scc_reg_access reg_struct;
    scc_return_t   scc_return = -1;
    int            error_code = IOCTL_SCC_OK;

    /* Try to copy user's reg_struct */
    if (copy_from_user(&reg_struct, (void *)scc_data, sizeof(reg_struct))) {
#ifdef SCC_DEBUG
        printk("SCC TEST: Error reading reg struct from user\n");
#endif
        error_code = -IOCTL_SCC_IMPROPER_ADDR;
    }
    else {

        /* call the real driver here */
        scc_return = scc_write_register(reg_struct.reg_offset,
                                         reg_struct.reg_data);
        if (scc_return != SCC_RET_OK) {
            error_code = -IOCTL_SCC_IMPROPER_ADDR;
        }
    }

    reg_struct.function_return_code = scc_return;
    copy_to_user((void *)scc_data, &reg_struct, sizeof(reg_struct));
    return error_code;
}


/*****************************************************************************/
/* fn scc_test_cipher()                                                      */
/*****************************************************************************/
/**
 * FUNC. scc_test_cipher
 * DESC. This function does the set up and the initiation of the Triple DES
 * (3DES) Encryption or Decryption depending on what has been requested by
 * application code (user space).
 *
 * Precondition: Application code has requested a 3DES Encryption/Decryption.
 * Postcondition: The SCC registers are properly set up and 3DES is initiated
 * for the SCC.
 *
 * Parameters:
 * cmd - SCC_ENCRYPT = Encryption or SCC_DECRYPT = Decryption
 * scc_data - Input struct provided by application code.
 */
static int
scc_test_cipher (uint32_t cmd, unsigned long scc_data)
{
    scc_encrypt_decrypt cipher_struct; /* local copy */
    uint32_t* area_in = 0;
    uint32_t* area_out = 0;
    scc_enc_dec_t enc_dec = SCC_DECRYPT;
    char* input;
    char* output;
    scc_return_t return_code = -1;

    int error_code = IOCTL_SCC_OK;

    if (copy_from_user(&cipher_struct,
                       (void *)scc_data, sizeof(cipher_struct))) {
#ifdef SCC_DEBUG
        printk("SCC TEST: Error reading cipher struct from user\n");
#endif
        error_code = -IOCTL_SCC_IMPROPER_ADDR;
    }
    else {

        area_in = kmalloc(cipher_struct.data_in_length+16, GFP_USER);

        area_out = kmalloc(cipher_struct.data_out_length+16, GFP_USER);

        if (cmd == SCC_TEST_ENCRYPT) {
            enc_dec = SCC_ENCRYPT;
        }

        if (!area_in || !area_out) {
            error_code = -IOCTL_SCC_NO_MEMORY;
        }
        else {
            int copy_code = 0;

            /* For testing, set up internal pointers to reflect the block/word
               offset of user memory. */
            input = (char*)area_in + (int)(cipher_struct.data_in)%8;
            output = (char*)area_out + (int)(cipher_struct.data_out)%8;
#ifdef SCC_DEBUG
            printk("SCC TEST: Input: 0x%p, Output: 0x%p\n", input, output);
#endif

            copy_code = copy_from_user(input,
                                       cipher_struct.data_in,
                                       cipher_struct.data_in_length);

            if (copy_code != 0) {
                error_code = -IOCTL_SCC_NO_MEMORY;
            }
            else {
#ifdef SCC_DEBUG
                printk("SCC TEST: Running cipher of %ld bytes.\n",
                       cipher_struct.data_in_length);
#endif
                /* call the real driver here */
                return_code =
                    scc_crypt(cipher_struct.data_in_length,
                              input,
#if 1
                              ((cipher_struct.crypto_mode == SCC_CBC_MODE)) ?
                              (char*)&cipher_struct.init_vector : NULL,
#else
                              (char*)&cipher_struct.init_vector,
#endif
                              enc_dec,
                              cipher_struct.crypto_mode,
                              cipher_struct.check_mode,
                              output,
                              &cipher_struct.data_out_length);

                if (return_code == SCC_RET_OK) {
                    copy_code = copy_to_user(cipher_struct.data_out,
                                             output,
                                             cipher_struct.data_out_length);
                }
#ifdef SCC_DEBUG
                else {
                    error_code = -IOCTL_SCC_FAILURE;
                    printk("SCC TEST: scc_crypt returned %d\n", return_code);
                }
#endif

                if (copy_code != 0 || return_code != 0) {
#ifdef SCC_DEBUG
                    if (copy_code) {
                        printk("SCC TEST: copy_to_user returned %d\n",
                               copy_code);
                    }
#endif
                    error_code = ENOMEM;
                }
            }

        } /* else allocated memory */

    } /* else copy of user struct succeeded */

    cipher_struct.function_return_code = return_code;
    copy_to_user((void *)scc_data, &cipher_struct, sizeof(cipher_struct));


    /* clean up */
    if (area_in) {
        kfree(area_in);
    }
    if (area_out) {
        kfree(area_out);
    }

    return error_code;
}


/*****************************************************************************/
/* fn scc_test_zeroize()                                                     */
/*****************************************************************************/
/**
 * Have the driver Zeroize the SCC memory
 *
 * @return 0 on success, -errno on failure
 */
static int
scc_test_zeroize(unsigned long scc_data) {
    scc_return_t return_code;

    return_code = scc_zeroize_memories();

    return copy_to_user((void*)scc_data, &return_code, sizeof(return_code));
}


static int
scc_test_alloc_slot(unsigned long cmd, unsigned long scc_data)
{
    scc_alloc_slot_access acc;
    int status;

    status = copy_from_user(&acc, (void*)scc_data, sizeof(acc));

    if (status == 0) {
        acc.scc_status = scc_alloc_slot(acc.value_size_bytes,
                                        acc.owner_id, &acc.slot);
        status = copy_to_user((void*)scc_data, &acc, sizeof(acc));
    }

    return status;
}


static int
scc_test_dealloc_slot(unsigned long cmd, unsigned long scc_data)
{
    scc_dealloc_slot_access acc;
    int status;

    status = copy_from_user(&acc, (void*)scc_data, sizeof(acc));
    if (status == 0) {
        acc.scc_status = scc_dealloc_slot(acc.owner_id, acc.slot);
        status = copy_to_user((void*)scc_data, &acc, sizeof(acc));
    }

    return status;
}


/*****************************************************************************/
/* fn scc_test_load_slot()                                                   */
/*****************************************************************************/
static int
scc_test_load_slot(unsigned long cmd, unsigned long scc_data)
{
    scc_load_slot_access acc;
    int status;

    uint32_t address;
    uint32_t key_size;
    uint32_t slot_size;
    uint8_t* key_data = NULL;

    status = copy_from_user(&acc, (void*)scc_data, sizeof(acc));

    if (status == 0) {
        /* First get RED data size for this slot. */
        acc.scc_status = scc_get_slot_info(acc.owner_id, acc.slot,
                                           &address, &key_size, &slot_size);

        if (acc.scc_status == SCC_RET_OK) {
            key_data = kmalloc(acc.key_data_length, GFP_KERNEL);

            if (key_data == NULL) {
                /* Not driver problem, just transitory test problem. */
                acc.scc_status = SCC_RET_INSUFFICIENT_SPACE;
            } else {
                status = copy_from_user(key_data, (void*)acc.key_data,
                                        acc.key_data_length);
                if (status == 0) {
                    if (acc.key_is_red) {
                        acc.scc_status = scc_load_slot(acc.owner_id,
                                                       acc.slot,
                                                       key_data,
                                                       acc.key_data_length);
                    } else {
                        acc.scc_status = scc_decrypt_slot(acc.owner_id,
                                                          acc.slot,
                                                          acc.key_data_length,
                                                          key_data);
                    }
                }
            }
        } /* get_slot_info was ok */

        status = copy_to_user((void*)scc_data, &acc, sizeof(acc));
    }

    return status;
}


/*****************************************************************************/
/* fn scc_test_encrypt_slot()                                                */
/*****************************************************************************/
static int
scc_test_encrypt_slot(unsigned long cmd, unsigned long scc_data)
{
    scc_encrypt_slot_access acc;
    int status;

    uint32_t address;
    uint32_t key_size;
    uint8_t* key_data = NULL;

    status = copy_from_user(&acc, (void*)scc_data, sizeof(acc));

    if (status == 0) {
        /* First get RED data size for this slot. */
        acc.scc_status = scc_get_slot_info(acc.owner_id, acc.slot,
                                           &address, &key_size, NULL);

        if (acc.scc_status == SCC_RET_OK) {
            key_data = kmalloc(acc.key_data_length, GFP_KERNEL);

            if (key_data == NULL) {
                /* Not driver problem, just transitory test problem. */
                acc.scc_status = SCC_RET_INSUFFICIENT_SPACE;
            } else {
                acc.scc_status = scc_encrypt_slot(acc.owner_id,
                                                  acc.slot,
                                                  acc.key_data_length,
                                                  key_data);
                if (acc.scc_status == SCC_RET_OK) {
                    status = copy_to_user(acc.key_data, key_data,
                                          acc.key_data_length);
                }
            }
        } /* get_slot_info was ok */

        if (status == 0) {
            status = copy_to_user((void*)scc_data, &acc, sizeof(acc));
        }
    }

    return status;
}


/*****************************************************************************/
/* fn scc_test_get_slot_info()                                               */
/*****************************************************************************/
static int
scc_test_get_slot_info(unsigned long cmd, unsigned long scc_data)
{
    scc_get_slot_info_access acc;
    int status;

    status = copy_from_user(&acc, (void*)scc_data, sizeof(acc));

    if (status == 0) {
        acc.scc_status = scc_get_slot_info(acc.owner_id, acc.slot,
                                           &acc.address,
                                           &acc.value_size_bytes,
                                           &acc.slot_size_bytes);
        status = copy_to_user((void*)scc_data, &acc, sizeof(acc));
    }

    return status;
}


/*****************************************************************************/
/* fn setup_user_driver_interaction()                                        */
/*****************************************************************************/
/**
 * Register the driver with the kernel.
 *
 * Call @c register_chrdev() to set this driver up as being
 * responsible for the SCC.  Save the major number for the driver
 * in scc_test_major_node.
 *
 * Called from #scc_test_init()
 *
 * @return 0 on success, -errno on failure.
 */
static int
setup_user_driver_interaction(void)
{
    struct class_device *temp_class;
    int result;
    int error_code = 0;

    /* Tell Linux kernel the user interface to the driver looks like */
    result = register_chrdev(scc_test_major_node, SCC_TEST_DRIVER_NAME,
                             &scc_test_fops);
    if (result < 0) {
        /* failure ! */
#ifdef SCC_DEBUG
        printk ("SCC TEST Driver: register device driver failed: %d\n",
                result);
#endif
        return result;
    }

    /* Save the major node value */
    if (scc_test_major_node == 0) {
        /* We passed in a zero value, then one was assigned to us.  */
        scc_test_major_node = result;
    }

	scc_tm_class = class_create(THIS_MODULE, SCC_TEST_DRIVER_NAME);
	if (IS_ERR(scc_tm_class)) {
		printk(KERN_ERR "Error creating scc test module class.\n");
		unregister_chrdev(scc_test_major_node, SCC_TEST_DRIVER_NAME);
		class_device_destroy(scc_tm_class, MKDEV(scc_test_major_node, 0));
		return PTR_ERR(scc_tm_class);
	}
 
	temp_class = class_device_create(scc_tm_class, NULL,
					     MKDEV(scc_test_major_node, 0), NULL,
					     SCC_TEST_DRIVER_NAME);
	if (IS_ERR(temp_class)) {
		printk(KERN_ERR "Error creating scc test module class device.\n");
		class_device_destroy(scc_tm_class, MKDEV(scc_test_major_node, 0));
		class_destroy(scc_tm_class);
		unregister_chrdev(scc_test_major_node, SCC_TEST_DRIVER_NAME);
		return -1;
	}

#ifdef SCC_DEBUG
    printk("SCC TEST Driver:  Major node is %d\n", scc_test_major_node);
#endif

    return error_code;
}


/*****************************************************************************/
/* fn scc_test_report_failure()                                              */
/*****************************************************************************/
/** Let the console know that the SCC has reported a security failure */
static void
scc_test_report_failure(void)
{
    printk("SCC TEST Driver: SMN reported alarm\n");
}
