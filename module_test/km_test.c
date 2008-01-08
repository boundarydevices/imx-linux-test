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

#include "../test/mxc_sahara_test/api_tests.h"
#include "km_test.h"

OS_DEV_INIT_DCL(km_test_init);
OS_DEV_SHUTDOWN_DCL(km_test_cleanup);
OS_DEV_OPEN_DCL(km_test_open);
OS_DEV_WRITE_DCL(km_test_write);
OS_DEV_CLOSE_DCL(km_test_close);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Freescale Semiconductor");
MODULE_DESCRIPTION("Test Device Driver for SAHARA2 Driver");

static os_driver_reg_t reg_handle;
static int Major = 0;

OS_DEV_INIT(km_test_init)
{
	os_error_code code = OS_ERROR_FAIL_S;
	int retval;

	os_driver_init_registration(reg_handle);
	os_driver_add_registration(reg_handle, OS_FN_OPEN,
				   OS_DEV_OPEN_REF(km_test_open));
	os_driver_add_registration(reg_handle, OS_FN_WRITE,
				   OS_DEV_WRITE_REF(km_test_write));
	os_driver_add_registration(reg_handle, OS_FN_CLOSE,
				   OS_DEV_CLOSE_REF(km_test_close));
	code = os_driver_complete_registration(reg_handle, Major,
					       KM_TEST_DRIVER_NAME);

	if (code == OS_ERROR_FAIL_S) {
		os_printk("km_test driver initialization failed: %d\n", code);
		retval = FALSE;
	} else {
		Major = code;
		retval = TRUE;
	}

	os_dev_init_return(retval);
}

OS_DEV_SHUTDOWN(km_test_cleanup)
{
#ifndef PORTABLE_OS_VERSION
	/* Old way */
	os_driver_remove_registration(Major, KM_TEST_DRIVER_NAME);
#else
	os_driver_remove_registration(reg_handle);
#endif

	os_dev_shutdown_return(TRUE);
}

OS_DEV_OPEN(km_test_open)
{
	fsl_shw_uco_t *user_ctx;
	int retval = OS_ERROR_FAIL_S;

	os_dev_set_user_private(NULL);	/* in case of error */

	user_ctx = os_alloc_memory(sizeof(*user_ctx), GFP_USER);
	if (user_ctx == NULL) {
		retval = OS_ERROR_NO_MEMORY_S;
	} else {
		fsl_shw_return_t code;

		/* Set Results Pool size to 10 */
		fsl_shw_uco_init(user_ctx, 10);
		fsl_shw_uco_set_flags(user_ctx, FSL_UCO_BLOCKING_MODE);
		code = fsl_shw_register_user(user_ctx);
		if (code == FSL_RETURN_OK_S) {
			os_dev_set_user_private(user_ctx);
			retval = OS_ERROR_OK_S;
		} else {
			os_free_memory(user_ctx);
		}

	}

	os_dev_open_return(retval);
}

/*
 * Any write to the device will kick off the specified test(s).
 *
 * The write() does not complete until @a run_tests() has finished.
 */
OS_DEV_WRITE(km_test_write)
{
	fsl_shw_uco_t *user_ctx = os_dev_get_user_private();
	long retval = OS_ERROR_FAIL_S;
	char *test_string;
	uint32_t passed_count = 0;
	uint32_t failed_count = 0;

	if (os_dev_get_count() == 0) {
		retval = 0;
	} else {
		/* Get memory to hold user string plus NUL termination. */
		test_string = os_alloc_memory(1 + os_dev_get_count(), GFP_USER);

		if (test_string == NULL) {
			retval = OS_ERROR_NO_MEMORY_S;
		} else {
			/* Null-terminate the string */
			test_string[os_dev_get_count()] = 0;

			retval = os_copy_from_user(test_string, (void *)
						   os_dev_get_user_buffer(),
						   os_dev_get_count());

			if (retval == FSL_RETURN_OK_S) {
				os_printk("km_test: running tests %s\n",
					  test_string);

				run_tests(user_ctx, test_string, &passed_count,
					  &failed_count);
				os_printk
				    ("Total km_test run: %d passed, %d failed\n",
				     passed_count, failed_count);

				retval = os_dev_get_count();
			}

			os_free_memory(test_string);
		}		/* else memory allocation succeeded */
	}			/* else count was not 0 */

	os_dev_write_return(retval);
}

OS_DEV_CLOSE(km_test_close)
{
	fsl_shw_uco_t *user_ctx = os_dev_get_user_private();

	if (user_ctx != NULL) {
		/* somehow delay until async tests finish? */
		fsl_shw_deregister_user(user_ctx);
		os_free_memory(user_ctx);
	}

	os_dev_close_return(OS_ERROR_OK_S);
}
