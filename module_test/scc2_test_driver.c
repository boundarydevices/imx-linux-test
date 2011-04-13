/*
 * Copyright (C) 2005-2009, 2011 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/**
 * @file scc2_test_driver.c
 * @brief This is a test driver for the SCC2.
 *
 * This driver and its associated reference test program are intended
 * to demonstrate the use of the SCC2 block in various ways.
 *
 * The driver runs in a Linux environment.  Portation notes are
 * included at various points which should be helpful in moving the
 * code to a different environment.
 *
 */

#include "../include/scc2_test_driver.h"


MODULE_AUTHOR("Freescale Semiconductor");
MODULE_DESCRIPTION("Test Device Driver for SCC2 (SMN/SCM) Driver");

MODULE_LICENSE("GPL");

/** Create a place to track/notify sleeping processes */
DECLARE_WAIT_QUEUE_HEAD(waitQueue);

/* Mutex to prevent usage of the ioctl function by more than 1 user at a time */
DEFINE_MUTEX(scc2_test_mutex);

OS_DEV_INIT_DCL(scc2_test_init);
OS_DEV_SHUTDOWN_DCL(scc2_test_cleanup);
OS_DEV_OPEN_DCL(scc2_test_open);
OS_DEV_IOCTL_DCL(scc2_test_ioctl);
OS_DEV_CLOSE_DCL(scc2_test_release);

static os_driver_reg_t reg_handle;

/** The /dev/node value for user-kernel interaction */
int scc2_test_major_node = 0;


/** saved-off pointer of configuration information */
scc_config_t* scc_cfg;

static int scc2_test_get_configuration(unsigned long scc_data);
static int scc2_test_read_register(unsigned long scc_data);
static int scc2_test_write_register(unsigned long scc_data);
static os_error_code setup_user_driver_interaction(void);
static void scc2_test_report_failure(void);

static int scc2_test_alloc_part(unsigned long scc_data);
static int scc2_test_engage_part(unsigned long scc_data);
static int scc2_test_encrypt_part(unsigned long cmd, unsigned long scc_data);
static int scc2_test_release_part(unsigned long scc_data);
static int scc2_test_load_data(unsigned long scc_data);
static int scc2_test_read_data(unsigned long scc_data);

extern int scc2_test_major_node;


/***********************************************************************
 * scc2_test_init()                                                    *
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
OS_DEV_INIT(scc2_test_init)
{
    os_error_code      error_code = 0;
    uint32_t smn_status;

    /* call the real driver here */
    scc_cfg = scc_get_configuration();

    /* call the real driver here */
    if (! (error_code = scc_read_register(SMN_STATUS_REG, &smn_status))) {

        /* set up driver as character driver */
        if (! (error_code = setup_user_driver_interaction())) {

            /* Determine status of SCC */
            if ((smn_status & SMN_STATUS_STATE_MASK) == SMN_STATE_FAIL) {
                printk("SCC2 TEST Driver: SCC is in Fail mode\n");
            }
            else {
                printk("SCC2 TEST Driver: Ready\n");
            }
        }
        else {
        }
    }
    else {
        printk("SCC2 TEST: Could not read SMN_STATUS register: %d\n",
               error_code);
    }

    scc_monitor_security_failure(scc2_test_report_failure);

    /* any errors, we must undo what we've done */
    if (error_code) {
        scc2_test_cleanup();
    }

    os_dev_init_return(error_code);
}


/***********************************************************************
 * scc2_test_open()                                                   *
 **********************************************************************/
/**
 * Driver interface function for open() system call.
 *
 * This function increments the "IN USE" count.  It is called during
 * an open().  The interface is defined by the Linux DDI/DKI.
 *
 * @return 0 if successful, error code if not
 */
OS_DEV_OPEN(scc2_test_open)
{
    os_dev_open_return(OS_ERROR_OK_S);
}


/***********************************************************************
 * scc2_test_release()                                                 *
 **********************************************************************/
/**
 * Driver interface function for close() system call.
 *
 * This function decrements the "IN USE" count.  It is called during a
 * close().  The interface is defined by the Linux DDI/DKI.
 *
 *
 * @return 0 (always - errors are ignored)
 */
OS_DEV_CLOSE(scc2_test_release)
{
    os_dev_close_return(OS_ERROR_OK_S);
}


/***********************************************************************
 * scc2_test_cleanup()                                                 *
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
 * during error handling from #scc2_test_init().
 *
 */
OS_DEV_SHUTDOWN(scc2_test_cleanup)
{
    os_driver_remove_registration(reg_handle);

    pr_debug("SCC2 TEST: Cleaned up\n");

    os_dev_shutdown_return(TRUE);
}


/***********************************************************************
 * scc2_test_ioctl()                                                   *
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
 * - #SCC2_GET_CONFIGURATION - Return driver, SMN, and SCM versions, memory
      sizes, block size.
 * - #SCC_READ_REG - Read register from SCC.
 * - #SCC2_WRITE_REG - Write register to SCC.
 * - #SCC2_ENCRYPT - Encrypt Red memory into Black memory.
 * - #SCC2_DECRYPT - Decrypt Black memory into Red memory.
 *
 * @pre Application code supplies a command with the related data (via the
 * scc_data struct)
 *
 * @post A specific action is performed based on the requested command.
 *
 * @return 0 or an error code (IOCTL_SCC2_xxx)
 */
OS_DEV_IOCTL(scc2_test_ioctl)
{
    os_error_code error_code = OS_ERROR_OK_S;
    unsigned cmd = os_dev_get_ioctl_op();
    unsigned long scc_data = os_dev_get_ioctl_arg();

	mutex_lock(&scc2_test_mutex);

    switch (cmd) {
    case SCC2_TEST_GET_CONFIGURATION:
        error_code = scc2_test_get_configuration(scc_data);
        break;

    case SCC2_TEST_READ_REG:
        error_code = scc2_test_read_register(scc_data);
        break;

    case SCC2_TEST_WRITE_REG:
        error_code = scc2_test_write_register(scc_data);
        break;

    case SCC2_TEST_SET_ALARM:
        scc_set_sw_alarm();     /* call the real driver here */
        break;

    case SCC2_TEST_ALLOC_PART:
        error_code = scc2_test_alloc_part(scc_data);
        break;

    case SCC2_TEST_ENGAGE_PART:
        error_code = scc2_test_engage_part(scc_data);
        break;

    case SCC2_TEST_ENCRYPT_PART:
    case SCC2_TEST_DECRYPT_PART:
        error_code = scc2_test_encrypt_part(cmd, scc_data);
        break;

    case SCC2_TEST_LOAD_PART:
        error_code = scc2_test_load_data(scc_data);
        break;

    case SCC2_TEST_READ_PART:
        error_code = scc2_test_read_data(scc_data);
        break;

    case SCC2_TEST_RELEASE_PART:
        error_code = scc2_test_release_part(scc_data);
        break;

    default:
#ifdef SCC2_DEBUG
        printk("SCC2 TEST: Error in ioctl(): (0x%x) is an invalid "
               "command (0x%x,0x%x)\n", cmd, SCC2_TEST_GET_CONFIGURATION,
               SCC2_TEST_ALLOC_SLOT);
        printk("uint64 is %d, alloc is %d\n", sizeof(uint64_t),
               sizeof(scc_alloc_slot_access));
#endif
        error_code = -IOCTL_SCC2_INVALID_CMD;
        break;

    } /* End switch */

	mutex_unlock(&scc2_test_mutex);

    os_dev_ioctl_return(error_code);
}


/***********************************************************************
 * scc2_test_get_configuration()                                       *
 **********************************************************************/
/**
 * Internal routine to handle ioctl command #SCC2_GET_CONFIGURATION.
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
scc2_test_get_configuration(unsigned long scc_data)
{
    int error_code = IOCTL_SCC2_OK;

#ifdef SCC2_DEBUG
    printk("SCC2 TEST: Configuration\n");
#endif

    /* now copy out (write) the data into user space */
    /** @todo make sure scc_get_configuration never returns null! */
    if (copy_to_user((void *)scc_data, scc_get_configuration(),
                     sizeof(scc_config_t))) {
#ifdef SCC2_DEBUG
        printk("SCC2 TEST: Error writing data to user\n");
#endif
        error_code = -IOCTL_SCC2_IMPROPER_ADDR;
    }

    return error_code;

} /* scc2_get_configuration */


/***********************************************************************
 * scc2_test_read_register()                                           *
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
scc2_test_read_register(unsigned long scc_data)
{
    scc_reg_access reg_struct;
    scc_return_t   scc_return = -1;
    int            error_code = IOCTL_SCC2_OK;
    unsigned long  retval;

    if (copy_from_user(&reg_struct, (void *)scc_data, sizeof(reg_struct))) {
#ifdef SCC2_DEBUG
        printk("SCC2 TEST: Error reading reg struct from user\n");
#endif
        error_code = -IOCTL_SCC2_IMPROPER_ADDR;
    }

    else {
        /* call the real driver here */
        scc_return = scc_read_register(reg_struct.reg_offset,
                                       &reg_struct.reg_data);
        if (scc_return != SCC_RET_OK) {
            error_code = -IOCTL_SCC2_IMPROPER_ADDR;
        }
    }

    reg_struct.function_return_code = scc_return;
    retval = copy_to_user((void *)scc_data, &reg_struct, sizeof(reg_struct));
    return error_code;
}


/***********************************************************************
 * scc2_test_write_register()                                          *
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
scc2_test_write_register(unsigned long scc_data) {
    scc_reg_access reg_struct;
    scc_return_t   scc_return = -1;
    int            error_code = IOCTL_SCC2_OK;
    unsigned long  retval;

    /* Try to copy user's reg_struct */
    if (copy_from_user(&reg_struct, (void *)scc_data, sizeof(reg_struct))) {
#ifdef SCC2_DEBUG
        printk("SCC2 TEST: Error reading reg struct from user\n");
#endif
        error_code = -IOCTL_SCC2_IMPROPER_ADDR;
    }
    else {

        /* call the real driver here */
        scc_return = scc_write_register(reg_struct.reg_offset,
                                        reg_struct.reg_data);
        if (scc_return != SCC_RET_OK) {
            error_code = -IOCTL_SCC2_IMPROPER_ADDR;
        }
    }

    reg_struct.function_return_code = scc_return;
    retval = copy_to_user((void *)scc_data, &reg_struct, sizeof(reg_struct));
    return error_code;
}


/*****************************************************************************/
/* fn scc2_test_alloc_part()                                                 */
/*****************************************************************************/
static int
scc2_test_alloc_part(unsigned long scc_data)
{
    scc_partition_access acc;
    int status;

    status = copy_from_user(&acc, (void*)scc_data, sizeof(acc));

    if (status == 0) {
        acc.scc_status = scc_allocate_partition(acc.smid,
                                                 &acc.part_no,
                                                 (void*)&acc.virt_address,
                                                 &acc.phys_address);
        status = copy_to_user((void*)scc_data, &acc, sizeof(acc));
    }

    return status;
}


/*****************************************************************************/
/* fn scc2_test_engage_part()                                                */
/*****************************************************************************/
static int
scc2_test_engage_part(unsigned long scc_data)
{
    uint8_t umid[16];
    scc_partition_access acc;
    int status;
    uint8_t* umid_ptr = NULL;

    printk(KERN_ALERT "calling copy_from_user\n");
    status = copy_from_user(&acc, (void*)scc_data, sizeof(acc));

    if (status == 0) {
        if (acc.umid != NULL) {
            printk(KERN_ALERT "calling copy_from_user\n");
            status = copy_from_user(umid, acc.umid, 16);
            umid_ptr = umid;
        }
        if (status == 0) {
            printk(KERN_ALERT "calling engage_partition\n");
            acc.scc_status = scc_engage_partition((void*)acc.virt_address,
                                                   umid_ptr,
                                                   acc.permissions);
            status = copy_to_user((void*)scc_data, &acc, sizeof(acc));
        }
    }

    return status;
}


/*****************************************************************************/
/* fn do_encrypt_part()                                                      */
/*****************************************************************************/
static int
do_encrypt_part(scc_part_cipher_access* acc, uint8_t* local_black,
                dma_addr_t black_phys)
{
    int status;
    uint32_t IV[4];
    uint32_t* iv_ptr = (uint32_t*)&(acc->iv);

    /* Build the IV */
    IV[0] = iv_ptr[0];
    IV[1] = iv_ptr[1];
    IV[2] = 0;
    IV[3] = 0;

    /* Perform the red -> black encryption */
    acc->scc_status = scc_encrypt_region(acc->virt_address, acc->red_offset,
                                         acc->size_bytes, (void*)black_phys,
                                         IV, SCC_CYPHER_MODE_CBC);

    /* Copy the result to user's memory */
    status = copy_to_user(acc->black_address, local_black, acc->size_bytes);

    return status;
}

/*****************************************************************************/
/* fn do_decrypt_part()                                                      */
/*****************************************************************************/
static int
do_decrypt_part(scc_part_cipher_access* acc, uint8_t* local_black,
                dma_addr_t black_phys)
{
    int status;
    uint32_t IV[4];
    uint32_t* iv_ptr = (uint32_t*)&(acc->iv);

    /* Build the IV */
    IV[0] = iv_ptr[0];
    IV[1] = iv_ptr[1];
    IV[2] = 0;
    IV[3] = 0;

    /* Copy the black data from user's memory */
    status = copy_from_user(local_black, acc->black_address, acc->size_bytes);

    if (status == 0) {
        /* Perform the black -> red decryption */
        acc->scc_status = scc_decrypt_region(acc->virt_address, acc->red_offset,
                                             acc->size_bytes, (void*)black_phys,
                                             IV, SCC_CYPHER_MODE_CBC);
    }

    return status;
}


/*****************************************************************************/
/* fn scc2_test_encrypt_part()                                               */
/*****************************************************************************/
static int
scc2_test_encrypt_part(unsigned long cmd, unsigned long scc_data)
{
    scc_part_cipher_access acc;
    int status;

    status = copy_from_user(&acc, (void*)scc_data, sizeof(acc));

    if (status == 0) {
        dma_addr_t black_phys;
        void* black = (void*)dma_alloc_coherent(NULL, acc.size_bytes, &black_phys, GFP_USER);

        if (black == NULL) {
            status = IOCTL_SCC2_NO_MEMORY;
        }

        if (status == 0) {
            if (cmd == SCC2_TEST_ENCRYPT_PART) {
                status = do_encrypt_part(&acc, black, black_phys);
            }
            else {
                status = do_decrypt_part(&acc, black, black_phys);
            }
        }

        if (black != NULL) {
            dma_free_coherent(NULL, acc.size_bytes, black, black_phys);
        }
        if (status == 0) {
            status = copy_to_user((void*)scc_data, &acc, sizeof(acc));
        }
    }

    return status;


}

static int
scc2_test_load_data(unsigned long scc_data)
{
    scc_part_cipher_access acc;
    int status;

    status = copy_from_user(&acc, (void*)scc_data, sizeof(acc));
    if (status == 0) {
        status = copy_from_user((void*)acc.virt_address + acc.red_offset,
                                acc.black_address,
                                acc.size_bytes);
        acc.scc_status = SCC_RET_OK;
    }

    return status;
}


static int
scc2_test_read_data(unsigned long scc_data)
{
    scc_part_cipher_access acc;
    int status;

    status = copy_from_user(&acc, (void*)scc_data, sizeof(acc));
    if (status == 0) {
        status = copy_to_user(acc.black_address,
                              (void*)acc.virt_address + acc.red_offset,
                              acc.size_bytes);
        acc.scc_status = SCC_RET_OK;
    }

    return status;
}


/*****************************************************************************/
/* fn scc2_test_release_part()                                               */
/*****************************************************************************/
static int
scc2_test_release_part(unsigned long scc_data)
{
    scc_partition_access acc;
    int status;

    status = copy_from_user(&acc, (void*)scc_data, sizeof(acc));

    if (status == 0) {
        acc.scc_status = scc_release_partition((void*)acc.virt_address);
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
 * in scc2_test_major_node.
 *
 * Called from #scc2_test_init()
 *
 * @return 0 on success, -errno on failure.
 */
static os_error_code
setup_user_driver_interaction(void)
{
    os_error_code code = OS_ERROR_OK_S;

    os_driver_init_registration(reg_handle);
    os_driver_add_registration(reg_handle, OS_FN_OPEN,
                               OS_DEV_OPEN_REF(scc2_test_open));
    os_driver_add_registration(reg_handle, OS_FN_IOCTL,
                               OS_DEV_IOCTL_REF(scc2_test_ioctl));
    os_driver_add_registration(reg_handle, OS_FN_CLOSE,
                               OS_DEV_CLOSE_REF(scc2_test_release));
    code = os_driver_complete_registration(reg_handle, scc2_test_major_node,
                                               SCC2_TEST_DRIVER_NAME);

    if (code != OS_ERROR_OK_S) {
        /* failure ! */
        os_printk("SCC2 TEST Driver: register device driver failed: %d\n",
                  code);

        return code;
    }

    /* Save the major node value */
    if (scc2_test_major_node == 0) {
        /* We passed in a zero value, then one was assigned to us.  */
        scc2_test_major_node = code;
    }

    pr_debug("SCC2 TEST Driver:  Major node is %d\n", scc2_test_major_node);

    return code;
}


/*****************************************************************************/
/* fn scc2_test_report_failure()                                             */
/*****************************************************************************/
/** Let the console know that the SCC has reported a security failure */
static void
scc2_test_report_failure(void)
{
    printk("SCC2 TEST Driver: SMN reported alarm\n");
}
