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

/*!
 * @file module_test/pmic_convity_test/utest9_set_callback_main.c
 * @brief Implementation of the PMIC Connectivity set callback unit tests.
 *
 * @ingroup PMIC
 */

#include <linux/module.h>
#include <linux/wait.h>
#include <linux/device.h>


#include "asm/arch/pmic_convity.h"	/* For PMIC Connectivity driver interface. */
#include "test_support.h"	/* For unit test support framework.        */


static DECLARE_WAIT_QUEUE_HEAD(queue_usb_det);


static void usbCallback(const PMIC_CONVITY_EVENTS event)
{
			
	wake_up(&queue_usb_det);
	return;
}


static PMIC_STATUS testcase(void)
{
	DEFINE_WAIT(wait);
	PMIC_CONVITY_HANDLE handle = (PMIC_CONVITY_HANDLE) NULL;
	PMIC_STATUS rc = PMIC_SUCCESS;
	PMIC_STATUS rc_all = PMIC_SUCCESS;


	/* First get handle to connectivity hardware. */
	rc = pmic_convity_open(&handle, USB);

	if (rc != PMIC_SUCCESS) {
		TRACEMSG("pmic_convity_open() returned error %d\n", rc);
		rc_all = PMIC_ERROR;
	} else {
		TRACEMSG
		    ("\nTesting pmic_convity_set_callback ...WAITING FOR PLUG IN / REMOVAL \n");
		rc = pmic_convity_set_callback(handle, usbCallback,
					       USB_DETECT_4V4_RISE |
					       USB_DETECT_4V4_FALL |
					       USB_DETECT_2V0_RISE |
					       USB_DETECT_2V0_FALL |
					       USB_DETECT_0V8_RISE |
					       USB_DETECT_0V8_FALL |
					       USB_DETECT_MINI_A |
					       USB_DETECT_MINI_B);
		if (rc == PMIC_SUCCESS) {
			prepare_to_wait(&queue_usb_det, &wait, TASK_UNINTERRUPTIBLE);
			schedule_timeout(500 * HZ);
			finish_wait(&queue_usb_det, &wait);
			
		} else {
			TRACEMSG("FAILED\n");
			rc_all = PMIC_ERROR;
		}

		rc = pmic_convity_close(handle);
		if (rc != PMIC_SUCCESS) {
			TRACEMSG("pmic_convity_close() returned error %d\n",
				 rc);
			rc_all = PMIC_ERROR;
		}
	}

	return rc_all;
}

/**************************************************************************
 * Module initialization and termination functions.
 **************************************************************************
 */

/*
 * Initialization and Exit
 */
static int __init pmic_convity_utest_init(void)
{
	TRACEMSG(KERN_INFO "PMIC Connectivity Unit Test loading\n");


	 if (testcase() == PMIC_SUCCESS) {
			TRACEMSG("\nDETECTED  EVENT: - TEST PASSED\n");
	} else {
			TRACEMSG("\n TEST FAILED\n");
			
	}

	return 0;
}

static void __exit pmic_convity_utest_exit(void)
{
	TRACEMSG(KERN_INFO "PMIC Connectivity Unit Test unloaded\n");
}

/*
 * Module entry points
 */

module_init(pmic_convity_utest_init);
module_exit(pmic_convity_utest_exit);

MODULE_DESCRIPTION("PMIC Connectivity Unit Tests");
MODULE_AUTHOR("FreeScale");
MODULE_LICENSE("GPL");
