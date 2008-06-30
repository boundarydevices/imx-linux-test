/*
 * Copyright 2005-2008 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*! @file rng_test_driver.c
 *
 * @brief This is a test driver for the RNG driver.
 *
 * This driver and its associated reference test program are intended
 * to demonstrate the use of and excercise the RNG block from kernel space.
 *
 * The test program also shows use of the RNG driver from user space.
 */

#include "../include/rng_test_driver.h"

OS_DEV_INIT_DCL(rng_test_init);
OS_DEV_SHUTDOWN_DCL(rng_test_cleanup);

OS_DEV_OPEN_DCL(rng_test_open);
OS_DEV_IOCTL_DCL(rng_test_ioctl);
OS_DEV_CLOSE_DCL(rng_test_release);

/*!****************************************************************************
 *
 *  Global / Static Variables
 *
 *****************************************************************************/

/*!
 *  Major node (user/device interaction value) for this driver.
 */
static int rng_test_major_node = RNG_TEST_MAJOR_NODE;

/*!
 *  Flag to know whether #register_chrdev() succeeded.  Useful if the driver
 *  is ever to be unloaded.  0 is not registered, non-zero is registered.
 */
static int rng_device_registered = 0;

/*!
 * OS-independent handle used for registering user interface of a driver.
 */
static os_driver_reg_t reg_handle;

/*!****************************************************************************
 *
 *  Function Implementations - Externally Accessible
 *
 *****************************************************************************/

/*!***************************************************************************/
/* fn rng_test_init()                                                       */
/*!***************************************************************************/
/*!
 * Initialize the driver.
 *
 * Bind this driver to the RNG device node.
 *
 * This is called during module load (insmod) or kernel init.
 */
	OS_DEV_INIT(rng_test_init)
	{
		os_error_code code;
	
#ifdef RNG_DEBUG
		os_printk("%s: Driver Initialization begins\n", RNG_TEST_DRIVER_NAME);
#endif
		code = rng_test_setup_user_driver_interaction();
	
#ifdef RNG_DEBUG
		os_printk("%s: Driver Completed: %d\n", RNG_TEST_DRIVER_NAME, code);
#endif
	
		os_dev_init_return(code);
	} /* rng_test_init */
	
	
	
	/***********************************************************************
	 * rng_test_open()													   *
	 **********************************************************************/
	/**
	 * Driver interface function for open() system call.
	 *
	 * Register the test user with the FSL SHW API.
	 */
	OS_DEV_OPEN(rng_test_open)
	{
		fsl_shw_uco_t* user_ctx = os_alloc_memory(sizeof(*user_ctx), 0);
		os_error_code code = OS_ERROR_NO_MEMORY_S;
	
		if (user_ctx != NULL) {
			fsl_shw_return_t ret;
	
			fsl_shw_uco_init(user_ctx, 20);
			ret = fsl_shw_register_user(user_ctx);
	
			if (ret != FSL_RETURN_OK_S) {
				code = OS_ERROR_FAIL_S;
			} else {
				os_dev_set_user_private(user_ctx);
				code = OS_ERROR_OK_S;
			}
		}
	
		os_dev_open_return(code);
	}
	
	
	/***********************************************************************
	 * rng_test_ioctl() 												   *
	 **********************************************************************/
	/**
	 * Driver interface function for ioctl() system call.
	 *
	 * This function serves as a control for the commands being
	 * passed by the application code.	Depending on what command has been
	 * sent, a specific function will occur.
	 *
	 * This routine handles the following valid commands:
	 *
	 * - #RNG_GET_CONFIGURATION - Return driver, SMN, and SCM versions, memory
	 sizes, block size.
	 * - #RNG_READ_REG - Read register from RNG.
	 * - #RNG_WRITE_REG - Write register to RNG.
	 *
	 * @pre Application code supplies a command with the related data (via the
	 * rng_data struct)
	 *
	 * @post A specific action is performed based on the requested command.
	 *
	 * @return os_error_code
	 */
	OS_DEV_IOCTL(rng_test_ioctl)
	{
		os_error_code  code = OS_ERROR_FAIL_S;
		fsl_shw_uco_t* user_ctx = os_dev_get_user_private();
	
		switch (os_dev_get_ioctl_op()) {
		case RNG_TEST_GET_RANDOM:
			if (user_ctx != NULL) {
				code = rng_test_get_random(user_ctx,
										   os_dev_get_ioctl_arg());
			}
			break;
	
		case RNG_TEST_ADD_ENTROPY:
			if (user_ctx != NULL) {
				code = rng_test_add_entropy(user_ctx,
											os_dev_get_ioctl_arg());
			}
			break;
	
		case RNG_TEST_READ_REG:
			code = rng_test_read_register(os_dev_get_ioctl_arg());
			break;
	
		case RNG_TEST_WRITE_REG:
			code = rng_test_write_register(os_dev_get_ioctl_arg());
			break;
	
		default:
			code = OS_ERROR_FAIL_S;
		}
	
		os_dev_ioctl_return(code);
	}
	
	
	/***********************************************************************
	 * rng_test_release()												   *
	 **********************************************************************/
	/**
	 * Driver interface function for close() system call.
	 *
	 * De-register from the FSL SHW API.  Free associated memory.
	 */
	OS_DEV_CLOSE(rng_test_release)
	{
		fsl_shw_uco_t* user_ctx = os_dev_get_user_private();
	
		if (user_ctx != NULL) {
			fsl_shw_deregister_user(user_ctx);
			os_free_memory(user_ctx);
			os_dev_set_user_private(NULL);
		}
	
		os_dev_close_return(OS_ERROR_OK_S);
	}
	
	
	/******************************************************************************
	 *
	 *	Function Implementations - IOCTL support
	 *
	 *****************************************************************************/
	
	/*****************************************************************************/
	/* fn rng_test_get_random() 												 */
	/*****************************************************************************/
	/**
	 * This function will retrieve entropy from the internal buffer of the RNG
	 * driver.	If not enough entropy is available, it will SLEEP until more is
	 * available.
	 *
	 *
	 * @return					 See #rng_return_t.
	 */
	static os_error_code
	rng_test_get_random(fsl_shw_uco_t* user_ctx, unsigned long rng_data)
	{
		rng_test_get_random_t get_struct;
		rng_return_t	 rng_return = -1;
		os_error_code	 code = OS_ERROR_BAD_ADDRESS_S;
		uint32_t*		 data_buffer = NULL;
		int 			 count_words;
	
		if (os_copy_from_user(&get_struct, (void *)rng_data, sizeof(get_struct))) {
#ifdef RNG_DEBUG
			os_printk("RNG TEST: Error reading get struct from user\n");
#endif
		} else {
			data_buffer = os_alloc_memory(get_struct.count_bytes, 0);
			count_words = (get_struct.count_bytes+sizeof(uint32_t)-1)/4;
			rng_return = fsl_shw_get_random(user_ctx, sizeof(uint32_t)*count_words,
											(uint8_t*) data_buffer);
	
			get_struct.function_return_code = rng_return;
	
			/* Copy return code back (by copying whole request structure. */
			code = os_copy_to_user((void*)rng_data, &get_struct,
								   sizeof(get_struct));
	
			/* If appropriate, copy back data. */
			if (rng_return == RNG_RET_OK) {
				code = os_copy_to_user(get_struct.random, data_buffer,
									   get_struct.count_bytes);
			}
	
			if (data_buffer != NULL) {
				os_free_memory(data_buffer);
			}
		}
	
	
		return code;
	} /* rng_test_get_random */
	
	
	/*****************************************************************************/
	/* fn rng_test_add_entropy()												*/
	/*****************************************************************************/
	/**
	 * This function will add @c randomness to the RNG.
	 *
	 * @param[in]  randomness		Some 'good' entropy to add to the RNG.
	 *
	 * @return					 See #rng_test_return_t.
	 */
	static os_error_code
	rng_test_add_entropy(fsl_shw_uco_t* user_ctx, unsigned long rng_data)
	{
		rng_test_add_entropy_t add_struct;
		uint8_t*			   entropy = NULL;
		os_error_code		   code;
	
		code = os_copy_from_user(&add_struct, (void *)rng_data,
								 sizeof(add_struct));
		if (code != 0) {
#ifdef RNG_DEBUG
			os_printk("RNG TEST: Error reading add struct from user\n");
#endif
		} else {
			entropy = os_alloc_memory(add_struct.count_bytes, 0);
			if (entropy != NULL) {
				add_struct.function_return_code =
					fsl_shw_add_entropy(user_ctx, add_struct.count_bytes,
										entropy);
				code = os_copy_to_user((void*)rng_data, &add_struct,
									   sizeof(add_struct));
				os_free_memory(entropy);
			} else {
#ifdef RNG_DEBUG
				os_printk("RNG TEST: Error allocating %d bytes for entropy\n",
						  add_struct.count_bytes);
#endif
				code = OS_ERROR_NO_MEMORY_S;
			}
		}
	
		return code;
	} /* rng_test_add_entropy */
	
	
	/*****************************************************************************/
	/* fn rng_test_read_register()												*/
	/*****************************************************************************/
	/**
	 * Read value from an RNG register.
	 * The offset will be checked for validity (range) as well as whether it is
	 * accessible at the time of the call.
	 *
	 * @param[in]	register_offset  The (byte) offset within the RNG block
	 *								 of the register to be queried.  See
	 *								 @ref rngregs for meanings.
	 * @param[out]	value			 Pointer to where value from the register
	 *								 should be placed.
	 *
	 * @return					 See #rng_return_t.
	 *
	 */
	static os_error_code
	rng_test_read_register(unsigned long rng_data)
	{
		rng_test_reg_access_t reg_struct;
		rng_return_t	 rng_return = -1;
		os_error_code	 code;
	
		code = os_copy_from_user(&reg_struct, (void *)rng_data,
								 sizeof(reg_struct));
		if (code != OS_ERROR_OK_S) {
#ifdef RNG_DEBUG
			os_printk("RNG TEST: Error reading reg struct from user\n");
#endif
		} else {
			/* call the real driver here */
			rng_return = rng_read_register(reg_struct.reg_offset,
										   &reg_struct.reg_data);
			reg_struct.function_return_code = rng_return;
	
			code = os_copy_to_user((void *)rng_data, &reg_struct,
								   sizeof(reg_struct));
		}
	
		return code;
	} /* rng_test_read_register */
	
	
	/*****************************************************************************/
	/* fn rng_test_write_register() 											*/
	/*****************************************************************************/
	/**
	 * Write a new value into an RNG register.
	 *
	 * The offset will be checked for validity (range) as well as whether it is
	 * accessible at the time of the call.
	 *
	 * @param[in]  register_offset	The (byte) offset within the RNG block
	 *								of the register to be modified.  See
	 *								@ref rngregs for meanings.
	 * @param[in]  value			The value to store into the register.
	 *
	 * @return					 See #rng_test_return_t.
	 *
	 */
	static os_error_code
	rng_test_write_register(unsigned long rng_data)
	{
		rng_test_reg_access_t reg_struct;
		rng_return_t	 rng_return = -1;
		os_error_code	 code;
	
		/* Try to copy user's reg_struct */
		code = os_copy_from_user(&reg_struct, (void *)rng_data,
								 sizeof(reg_struct));
		if (code != OS_ERROR_OK_S) {
#ifdef RNG_DEBUG
			os_printk("RNG TEST: Error reading reg struct from user\n");
#endif
		} else {
			/* call the real driver here */
			rng_return = rng_write_register(reg_struct.reg_offset,
											reg_struct.reg_data);
			reg_struct.function_return_code = rng_return;
			code = os_copy_to_user((void *)rng_data, &reg_struct,
								   sizeof(reg_struct));
		}
	
		return code;
	} /* rng_test_write_register */
	
	
	
	/******************************************************************************
	 *
	 *	Function Implementations - Strictly Internal
	 *
	 *****************************************************************************/
	
	
	/*****************************************************************************/
	/* fn setup_user_driver_interaction()										 */
	/*****************************************************************************/
	/**
	 * Register the driver as the driver for #RNG_TEST_MAJOR_NODE
	 *
	 * If RNG_TEST_MAJOR_NODE is zero, then make sure that #rng_test_major_node
	 * has the registered node value.
	 */
	static os_error_code
	rng_test_setup_user_driver_interaction(void)
	{
		os_error_code code = OS_ERROR_FAIL_S;
	
		os_driver_init_registration(reg_handle);
		os_driver_add_registration(reg_handle, OS_FN_OPEN,
								   OS_DEV_IOCTL_REF(rng_test_open));
		os_driver_add_registration(reg_handle, OS_FN_IOCTL,
								   OS_DEV_IOCTL_REF(rng_test_ioctl));
		os_driver_add_registration(reg_handle, OS_FN_CLOSE,
								   OS_DEV_IOCTL_REF(rng_test_release));
		code = os_driver_complete_registration(reg_handle,
											   rng_test_major_node,
											   RNG_TEST_DRIVER_NAME);
	
		if (code != OS_ERROR_OK_S) {
			/* failure ! */
#ifdef RNG_DEBUG
			os_printk ("RNG Test: register device driver failed: %d\n", code);
#endif
		} else {					  /* success */
			rng_device_registered = 1;
#ifdef RNG_DEBUG
			os_printk("RNG Test:  Major node is %d\n",
					  os_dev_driver_major_node(reg_handle));
#endif
		} /* else success */
	
		return code;
	} /* rng_test_setup_user_driver_interaction */
	
	
	/*****************************************************************************/
	/* fn rng_test_cleanup()													*/
	/*****************************************************************************/
	/**
	 * Prepare driver for exit.
	 *
	 * This is called during @c rmmod when the driver is unloading.  Try to undo
	 * whatever was done during #rng_test_init(), to make the machine be in the
	 * same state, if possible.
	 *
	 * Mask off RNG interrupts.  Put the RNG to sleep?
	 */
	OS_DEV_SHUTDOWN(rng_test_cleanup)
	{
		os_error_code code = OS_ERROR_OK_S;
	
		if (rng_device_registered) {
			/* turn off the mapping to the device special file */
			code = os_driver_remove_registration(reg_handle);
			rng_device_registered = 0;
		}
	
#ifdef RNG_DEBUG
		os_printk ("RNG Test: Cleaned up\n");
#endif
	
		os_dev_shutdown_return(code);
	} /* rng_test_cleanup */


MODULE_DESCRIPTION("Test Module for MXC drivers");
MODULE_LICENSE("GPL");
