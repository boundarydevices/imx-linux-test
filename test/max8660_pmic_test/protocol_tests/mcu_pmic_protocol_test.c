/*
 * Copyright 2008-2009 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file   mcu_pmic_protocol_test.c
 * @brief  mx35 mcu pmic Protocol driver testing.
 */

/* Standard Include Files */
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <linux/pmic_external.h>

#define TFAIL -1
#define TPASS 0

static char* test_name = "mx35 mcu pmic protocol";
#define tst_log(fmt, arg...) \
	printf("mx35 mcu pmic protocol test "fmt"\n", ##arg)
static
int VT_pmic_read(int fd, int reg, unsigned int *val) {
	register_info reg_info;
	reg_info.reg = reg;
	reg_info.reg_value = 0;

	int rv = ioctl(fd, PMIC_READ_REG, &reg_info);
	*val = reg_info.reg_value;
	if (rv != 0)
		return TFAIL;
	else
		return TPASS;
}

static
int VT_pmic_write(int fd, int reg, unsigned int val) {
	register_info reg_info;
	reg_info.reg = reg;
	reg_info.reg_value = val;

	int rv = ioctl(fd, PMIC_WRITE_REG, &reg_info);

	if (rv != 0)
		return TFAIL;
	else
		return TPASS;
}


int main(int argc, char **argv) {
	int VT_rv = TPASS;
	int fd;
	int reg;
	int ret;
	unsigned int value;
	tst_log("started");

	fd = open("/dev/pmic", O_RDWR);
	if (fd < 0) {

		tst_log("fail to open dev");
		VT_rv = TFAIL;
	}
	else {
	for (reg = REG_MCU_VERSION; reg <= REG_MAX8660_FORCE_PWM; reg ++) {

		ret = VT_pmic_read(fd, reg, &value);
		if (0 != ret) {

			tst_log("fail to read reg 0x%x", reg);
			VT_rv = TFAIL;
			continue;
		}
		sleep(1);
		ret = VT_pmic_write(fd, reg, value);
		if (0 != ret) {

			tst_log("fail to write reg 0x%x", reg);
			VT_rv = TFAIL;
		}
		tst_log("access reg 0x%x OK", reg);
		sleep(1);

	}
	}
	/* Print test Assertion */
	if (VT_rv == TPASS) {
		tst_log("worked as expected");
	} else {
		tst_log("did NOT work as expected\n");
	}

	return VT_rv;
}
