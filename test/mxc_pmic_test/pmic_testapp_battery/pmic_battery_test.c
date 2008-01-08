/* 
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved. 
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
 * @file   pmic_battery_test.c 
 * @brief  Test scenario C source PMIC.
 */

/*==============================================================================
                                        INCLUDE FILES
==============================================================================*/
/* Harness Specific Include Files. */
#include "test.h"

/* Verification Test Environment Include Files */
#include "pmic_battery_test.h"

/*==============================================================================
                                       GLOABAL VARIABLES
==============================================================================*/
extern char *TCID;
extern int fd;
/*==============================================================================
                                        LOCAL FUNCTIONS
==============================================================================*/
#define CHGR_REG_VOUT_MAX	8
#define CHGR_MAIN_DAC_CUR_MAX	8
#define CHGR_INT_TRCK_CUR_MAX	8
#define CHGR_COIN_VOUT_MAX	4
#define CHGR_OVOLT_THRESH_MAX	4

/*============================================================================*/
/*===== VT_pmic_batt_test =====*/
/**
@brief  PMIC Battery test scenario

@param  switch_fct
        Number test case.
  
@return On success - return PMIC_SUCCESS
        On failure - return the error code
*/
/*============================================================================*/
int VT_pmic_batt_test(int switch_fct)
{
	PMIC_STATUS status;
	int VT_rv = PMIC_SUCCESS;
	t_control control;
	int threshold;
	unsigned short c_current;
	int i, j;

	t_charger_setting tset1;
	memset(&tset1, 0x00, sizeof(t_charger_setting));

	t_eol_setting tset2;
	memset(&tset2, 0x00, sizeof(t_eol_setting));

	switch (switch_fct) {
	case 0:
		tst_resm(TINFO,
			 "Test main charger control function of PMIC Battery");
		tset1.on = TRUE;
		tset1.chgr = BATT_MAIN_CHGR;
		for (i = 0; i < CHGR_REG_VOUT_MAX; i++) {
			for (j = 0; j < CHGR_MAIN_DAC_CUR_MAX; j++) {
				tset1.c_voltage = i;
				tset1.c_current = j;

				tst_resm(TINFO, "VOLTAGE = %d\t CURRENT = %d\n",
					 tset1.c_voltage, tset1.c_current);
				status =
				    ioctl(fd, PMIC_BATT_CHARGER_CONTROL, &tset1);
				if (status != PMIC_SUCCESS) {
					VT_rv = status;
					tst_resm(TFAIL,
						 "Error in pmic_batt_enable_charger"
						 "(main charger). Error code: %d",
						 status);
				}
				
				tset1.c_voltage = 0;
				tset1.c_current = 0;

				status =
				    ioctl(fd, PMIC_BATT_GET_CHARGER, &tset1);
				if (status != PMIC_SUCCESS) {
					VT_rv = status;
					tst_resm(TFAIL,
						 "Error in pmic_batt_get_charger_setting"
						 "(main charger). Error code: %d",
						 status);
				}

				if (tset1.c_voltage != i) {
					VT_rv = PMIC_ERROR;
					tst_resm(TFAIL,
						 "Error PMIC battery control not"
						 " configured properly\n");
				}

			}
		}

		tset1.on = FALSE;
		tst_resm(TINFO,
			 "Test disable charger control function of PMIC Battery driver");
		status = ioctl(fd, PMIC_BATT_CHARGER_CONTROL, &tset1);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_disable_charger"
				 "(main charger). Error code: %d",
				 status);
		}

		tst_resm(TINFO,
			 "Test COIN cell charger control function of PMIC Battery");
		tset1.on = TRUE;
		tset1.chgr = BATT_CELL_CHGR;
		for (i = 0; i < CHGR_COIN_VOUT_MAX; i++) {
			tset1.c_voltage = i;

			tst_resm(TINFO, "VOLTAGE = %d\n", tset1.c_voltage);
			status = ioctl(fd, PMIC_BATT_CHARGER_CONTROL, &tset1);
			if (status != PMIC_SUCCESS) {
				VT_rv = status;
				tst_resm(TFAIL,
					 "Error in pmic_batt_enable_charger"
					 "(coincell charger). Error code: %d",
					 status);
			}

			status = ioctl(fd, PMIC_BATT_GET_CHARGER, &tset1);
			if (status != PMIC_SUCCESS) {
				VT_rv = status;
				tst_resm(TFAIL,
					 "Error in pmic_batt_get_charger_setting"
					 "(coincell charger). Error code: %d",
					 status);
			}

			if (tset1.c_voltage != i) {
				VT_rv = PMIC_ERROR;
				tst_resm(TFAIL,
					 "Error PMIC battery control not"
					 " configured properly\n");
			}
		}

		tset1.on = FALSE;
		tst_resm(TINFO,
			 "Test disable charger control function of PMIC Battery driver");
		status = ioctl(fd, PMIC_BATT_CHARGER_CONTROL, &tset1);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_disable_charger"
				 "(coincell charger). Error code: %d",
				 status);
		}

		tst_resm(TINFO,
			 "Test TRICKLE charger control function of PMIC Battery");
		tset1.on = TRUE;
		tset1.chgr = BATT_TRCKLE_CHGR;
		for (i = 0; i < CHGR_INT_TRCK_CUR_MAX; i++) {
			tset1.c_current = i;

			tst_resm(TINFO, "CURRENT = %d\n", tset1.c_current);
			status = ioctl(fd, PMIC_BATT_CHARGER_CONTROL, &tset1);
			if (status != PMIC_SUCCESS) {
				VT_rv = status;
				tst_resm(TFAIL,
					 "Error in pmic_batt_enable_charger"
					 "(coincell charger). Error code: %d",
					 status);
			}

			status = ioctl(fd, PMIC_BATT_GET_CHARGER, &tset1);
			if (status != PMIC_SUCCESS) {
				VT_rv = status;
				tst_resm(TFAIL,
					 "Error in pmic_batt_get_charger_setting"
					 "(coincell charger). Error code: %d",
					 status);
			}

			if (tset1.c_current != i) {
				VT_rv = PMIC_ERROR;
				tst_resm(TFAIL,
					 "Error PMIC battery control not"
					 " configured properly\n");
			}
		}

		tset1.on = FALSE;
		tst_resm(TINFO,
			 "Test disable charger control function of PMIC Battery driver");
		status = ioctl(fd, PMIC_BATT_CHARGER_CONTROL, &tset1);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_disable_charger"
				 "(coincell charger). Error code: %d",
				 status);
		}

		break;
	case 1:
		tst_resm(TINFO, "Test enable eol function of PMIC Battery");
		tset2.threshold = 0;
		for (i = 0; i < CHGR_OVOLT_THRESH_MAX; i++) {
			tset2.typical = i;

			tst_resm(TINFO, "THRESHOLD = %d\n", tset2.typical);

			tset2.enable = TRUE;
			status = ioctl(fd, PMIC_BATT_EOL_CONTROL, &tset2);
			if (status != PMIC_SUCCESS) {
				VT_rv = status;
				tst_resm(TFAIL,
					 "Error in pmic_batt_enable_eol. Error code: %d",
					 status);
			}
		}
		tset2.enable = FALSE;
		tst_resm(TINFO, "Test disable eol function of PMIC Battery");
		status = ioctl(fd, PMIC_BATT_EOL_CONTROL, &tset2);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_disable_eol. Error code: %d",
				 status);
		}

		break;
	case 2:
		tst_resm(TINFO, "Test led control function of PMIC Battery");

		status = ioctl(fd, PMIC_BATT_LED_CONTROL, TRUE);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_led_control enable. Error code: %d",
				 status);
		}
		sleep(1);
		status = ioctl(fd, PMIC_BATT_LED_CONTROL, FALSE);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_led_control disable. Error code: %d",
				 status);
		}

		break;
	case 3:
		tst_resm(TINFO,
			 "Test set reverse supply function of PMIC Battery");
		status = ioctl(fd, PMIC_BATT_REV_SUPP_CONTROL, TRUE);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_set_reverse_supply enable. Error code: %d",
				 status);
		}

		tst_resm(TINFO,
			 "Test set reverse supply function of PMIC Battery");
		status = ioctl(fd, PMIC_BATT_REV_SUPP_CONTROL, FALSE);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_set_reverse_supply disable. Error code: %d",
				 status);
		}

		tst_resm(TINFO,
			 "Test unregulatored function of PMIC Battery enable");
		status = ioctl(fd, PMIC_BATT_UNREG_CONTROL, TRUE);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_set_unregulated enable. Error code: %d",
				 status);
		}

		tst_resm(TINFO,
			 "Test unregulatored function of PMIC Battery disable");
		status = ioctl(fd, PMIC_BATT_UNREG_CONTROL, FALSE);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_set_unregulated disable. Error code: %d",
				 status);
		}

		break;

	case 4:
		tst_resm(TINFO,
			 "Test set out control function of PMIC Battery in CONTROL_BPFET_LOW");
		control = CONTROL_BPFET_LOW;
		status = ioctl(fd, PMIC_BATT_SET_OUT_CONTROL, control);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_set_out_control with"
				 "control = CONTROL_BPFET_LOW. Error code: %d",
				 status);
		}

		tst_resm(TINFO,
			 "Test set out control function of PMIC Battery in CONTROL_BPFET_HIGH");
		control = CONTROL_BPFET_HIGH;
		status = ioctl(fd, PMIC_BATT_SET_OUT_CONTROL, control);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_set_out_control with"
				 "control = CONTROL_BPFET_LOW. Error code: %d",
				 status);
		}

		tst_resm(TINFO,
			 "Test set out control function of PMIC Battery in CONTROL_HARDWARE");
		control = CONTROL_HARDWARE;
		status = ioctl(fd, PMIC_BATT_SET_OUT_CONTROL, control);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_set_out_control with"
				 "control = CONTROL_HARDWARE. Error code: %d",
				 status);
		}

		break;

	case 5:
		tst_resm(TINFO, "Test set threshold function of PMIC Battery");
		threshold = 3;
		status = ioctl(fd, PMIC_BATT_SET_THRESHOLD, threshold);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_set_threshold with"
				 "threshold = 3. Error code: %d",
				 status);
		}

		threshold = 2;
		status = ioctl(fd, PMIC_BATT_SET_THRESHOLD, threshold);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_set_threshold with"
				 "threshold = 2. Error code: %d",
				 status);
		}

		threshold = 0;
		status = ioctl(fd, PMIC_BATT_SET_THRESHOLD, threshold);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_set_threshold"
				 "with threshold = 0. Error code: %d",
				 status);
		}

		break;

	case 6:
		tst_resm(TINFO,
			 "Test get charger current function of PMIC Battery");
		status = ioctl(fd, PMIC_BATT_GET_CHARGER_CURRENT, &c_current);
		if (status != PMIC_SUCCESS) {
			VT_rv = status;
			tst_resm(TFAIL,
				 "Error in pmic_batt_get_charger_current. Error code: %d",
				 status);
		}
		printf("charger current : %d.\n", c_current);

		break;

	default:
		tst_resm(TINFO,
			 "Error in PMIC Battery Test: Unsupported operation");
		VT_rv = TFAIL;
	}
	return VT_rv;
}
