
/*!
 * @file pmic_testapp_light.c
 * @brief This is the main file of PMIC tests for Light and Backlight driver. 
 *
 * @ingroup MC13783_LIGHT
 */

/*
 * Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/types.h>		/* open() */
#include <sys/stat.h>		/* open() */
#include <fcntl.h>		/* open() */
#include <sys/ioctl.h>		/* ioctl() */
#include <unistd.h>		/* close() */
#include <stdio.h>		/* sscanf() & perror() */
#include <stdlib.h>		/* atoi() */

#include <asm/arch/pmic_external.h>
#include <asm/arch/pmic_light.h>

#include "../include/test.h"
#include "../include/usctest.h"

void testcase_display(void)
{
	printf("\n");
	printf("*******************************************************\n");
	printf("********** PMIC LIGHT Test Case options ***************\n");
	printf("*******************************************************\n");
	printf("\n");
	printf("*******************************************************\n");
	printf("***** '-T 1'  Enable Backlight ************************\n");
	printf("***** '-T 2'  Ramp up Backlight ***********************\n");
	printf("***** '-T 3'  Ramp down Backlight *********************\n");
	printf("***** '-T 4'  Automated Backlight test ****************\n");
	printf("***** '-T 5'  Start TC LED pattern ********************\n");
	printf("***** '-T 6'  Automated FUN test **********************\n");
	printf("***** '-T 7'  Start TC LED Indicator ******************\n");
	printf("***** '-T 8'  Automated TCLED INDICATOR test **********\n");
	printf("*******************************************************\n");
	printf("\n");
}

void bank_display(void)
{
	printf("\n");
	printf("*******************************************************\n");
	printf("********** PMIC LIGHT LED Bank options ****************\n");
	printf("*******************************************************\n");
	printf("\n");
	printf("*******************************************************\n");
	printf("***** '-B 1'  BANK # 1 *******************************\n");
	printf("***** '-B 2'  BANK # 2 *******************************\n");
	printf("***** '-B 3'  BANK # 3 *******************************\n");
	printf("*******************************************************\n");
	printf("\n");
}

void color_display(void)
{
	printf("\n");
	printf("*******************************************************\n");
	printf("********** PMIC LIGHT LED Color options ***************\n");
	printf("*******************************************************\n");
	printf("\n");
	printf("*******************************************************\n");
	printf("***** '-C 1'  RED ************************************\n");
	printf("***** '-C 2'  GREEN **********************************\n");
	printf("***** '-C 3'  BLUE ************************************\n");
	printf("*******************************************************\n");
	printf("\n");
}

void pattern_display(void)
{
	printf("\n");
	printf("*******************************************************\n");
	printf("********** PMIC LIGHT LED PATTERN options *************\n");
	printf("*******************************************************\n");
	printf("\n");
	printf("*******************************************************\n");
	printf("***** '-F 1'   BLENDED_RAMPS_SLOW *********************\n");
	printf("***** '-F 2'   BLENDED_RAMPS_FAST *********************\n");
	printf("***** '-F 3'   SAW_RAMPS_SLOW *************************\n");
	printf("***** '-F 4'   SAW_RAMPS_FAST *************************\n");
	printf("***** '-F 5'   BLENDED_BOWTIE_SLOW ********************\n");
	printf("***** '-F 6'   BLENDED_BOWTIE_FAST ********************\n");
	printf("***** '-F 7'   STROBE_SLOW ****************************\n");
	printf("***** '-F 8'   STROBE_FAST ****************************\n");
	printf("***** '-F 9'   CHASING_LIGHT_RGB_SLOW *****************\n");
	printf("***** '-F 10'  CHASING_LIGHT_RGB_FAST *****************\n");
	printf("***** '-F 11'  CHASING_LIGHT_BGR_SLOW *****************\n");
	printf("***** '-F 12'  CHASING_LIGHT_BGR_FAST *****************\n");
	printf("*******************************************************\n");
	printf("\n");
}

/*!
 * This function enable backlights
 * Main, Aux and Keypad
 *
 * @param        tcled_setting  the tcled setting
 *
 * @return       This function returns a void
 */
void enable_backlight(int fp, t_bklit_setting_param * setting)
{
	setting->channel = BACKLIGHT_LED1;
	setting->current_level = 4;
	setting->duty_cycle = 7;
	setting->mode = BACKLIGHT_CURRENT_CTRL_MODE;
	setting->cycle_time = 2;
	setting->strobe = BACKLIGHT_STROBE_NONE;
	setting->edge_slow = false;
	setting->en_dis = 0;
	setting->abms = 0;
	setting->abr = 0;
	ioctl(fp, PMIC_SET_BKLIT, setting);

	sleep(1);
	setting->channel = BACKLIGHT_LED2;
	ioctl(fp, PMIC_SET_BKLIT, setting);

	sleep(1);
	setting->channel = BACKLIGHT_LED3;
	ioctl(fp, PMIC_SET_BKLIT, setting);
}

/*!
 * This function disable backlights
 * Main, Aux and Keypad
 *
 * @param        tcled_setting  the tcled setting
 *
 * @return       This function returns a void
 */
void disable_backlight(int fp, t_bklit_setting_param * setting)
{
	setting->channel = BACKLIGHT_LED1;
	setting->current_level = 0;
	setting->duty_cycle = 0;
	setting->mode = BACKLIGHT_CURRENT_CTRL_MODE;
	setting->cycle_time = 0;
	setting->strobe = BACKLIGHT_STROBE_NONE;
	setting->edge_slow = false;
	ioctl(fp, PMIC_SET_BKLIT, setting);

	setting->channel = BACKLIGHT_LED2;
	ioctl(fp, PMIC_SET_BKLIT, setting);

	setting->channel = BACKLIGHT_LED3;
	ioctl(fp, PMIC_SET_BKLIT, setting);

}

/*!
 * This function start a specific fun pattern
 *
 * @param        fp             the file pointer
 * @param        tcled_setting  the tcled setting
 * @param        fun_param      the fun parameter
 *
 * @return       This function returns 0 if successful.
 */
int start_fun_pattern(int fp,
		      t_tcled_enable_param * tcled_setting,
		      t_fun_param * fun_param)
{
	tcled_setting->mode = TCLED_FUN_MODE;
	fun_param->bank = tcled_setting->bank;

	ioctl(fp, PMIC_TCLED_PATTERN, fun_param);
	ioctl(fp, PMIC_TCLED_ENABLE, tcled_setting);
	sleep(2);
	ioctl(fp, PMIC_TCLED_DISABLE, tcled_setting->bank);

	printf("Test Passed\n");
	return 0;
}

/*!
 * This is the unit test for the backlight.
 *
 * @param        fp             the file pointer
 * @param        tcled_setting  the tcled setting
 * @param        fun_param        the fun parameter
 *
 * @return       This function returns 0 if successful.
 */
int run_backlight_test(int fp, t_bklit_setting_param * setting)
{
	t_bklit_setting_param r_setting;

	if (ioctl(fp, PMIC_SET_BKLIT, setting)) {
		printf("Test Failed ...1!");
		return -1;
	}
	r_setting.channel = setting->channel;
	if (ioctl(fp, PMIC_GET_BKLIT, &r_setting)) {
		printf("Test Failed ...2!");
		return -1;
	}
	if ((r_setting.mode != setting->mode) ||
	    (r_setting.mode != setting->mode) ||
	    (r_setting.strobe != setting->strobe) ||
	    (r_setting.channel != setting->channel) ||
	    (r_setting.current_level != setting->current_level) ||
	    (r_setting.duty_cycle != setting->duty_cycle) ||
	    (r_setting.cycle_time != setting->cycle_time) ||
	    (r_setting.edge_slow != setting->edge_slow) ||
	    (r_setting.en_dis != setting->en_dis) ||
	    (r_setting.abms != setting->abms) ||
	    (r_setting.abr != setting->abr)) {
		printf("Test FAILED");
		printf("mode %d - %d\n", r_setting.mode, setting->mode);
		printf("strobe %d - %d\n", r_setting.strobe, setting->strobe);
		printf("channel %d - %d\n", r_setting.channel,
		       setting->channel);
		printf("current level %d - %d\n", r_setting.current_level,
		       setting->current_level);
		printf("duty cycle %d - %d\n", r_setting.duty_cycle,
		       setting->duty_cycle);
		printf("cycle time%d - %d\n", r_setting.cycle_time,
		       setting->cycle_time);
		printf("edge slow %d - %d\n", r_setting.edge_slow,
		       setting->edge_slow);
		printf("en dis %d - %d\n", r_setting.en_dis, setting->en_dis);
		printf("abms %d - %d\n", r_setting.abms, setting->abms);
		printf("abr %d - %d\n", r_setting.abr, setting->abr);
		return -1;
	}

	if (ioctl(fp, PMIC_RAMPUP_BKLIT, setting->channel))
		return -1;
	if (ioctl(fp, PMIC_OFF_RAMPUP_BKLIT, setting->channel))
		return -1;
	if (ioctl(fp, PMIC_RAMPDOWN_BKLIT, setting->channel))
		return -1;
	if (ioctl(fp, PMIC_OFF_RAMPDOWN_BKLIT, setting->channel))
		return -1;

	return 0;
}

/*!
 * This is the suite test for the backlight.
 *
 * @param        fp             the file pointer
 * @param        setting          the backlight setting
 *
 * @return       This function returns 0 if successful.
 */
int run_suite_backlight_test(int fp, t_bklit_setting_param * setting)
{
	for (setting->channel = BACKLIGHT_LED1;
	     setting->channel <= BACKLIGHT_LED3; setting->channel++) {
		for (setting->current_level = 0;
		     setting->current_level <= 7; setting->current_level++) {
			for (setting->duty_cycle = 0;
			     setting->duty_cycle <= 15; setting->duty_cycle++) {
				for (setting->cycle_time = 0;
				     setting->cycle_time <= 3;
				     setting->cycle_time++) {
					setting->edge_slow = 0;
					if (run_backlight_test(fp, setting) < 0) {
						return -1;
					}
					setting->edge_slow = 1;
					if (run_backlight_test(fp, setting) < 0) {
						return -1;
					}
				}
			}
		}
	}
	return 0;
}

/*!
 * This is the main test for the backlight.
 *
 * @param        fp             the file pointer
 * @param        setting          the backlight setting
 *
 * @return       This function returns 0 if successful.
 */
int backlight_test(int fp, t_bklit_setting_param * setting)
{
	printf("Backlight test\n");

	setting->mode = BACKLIGHT_CURRENT_CTRL_MODE;
	setting->strobe = BACKLIGHT_STROBE_NONE;
	setting->en_dis = 0;
	setting->abms = 0;
	setting->abr = 0;
	for (setting->mode = BACKLIGHT_CURRENT_CTRL_MODE;
	     setting->mode <= BACKLIGHT_TRIODE_MODE; setting->mode++) {
		if (run_suite_backlight_test(fp, setting) < 0) {
			printf("Test FAILED");
			return -1;
		}
	}

	setting->channel = BACKLIGHT_LED1;
	setting->current_level = 3;
	setting->duty_cycle = 10;
	setting->cycle_time = 2;
	setting->edge_slow = 0;
	setting->en_dis = 0;
	for (setting->mode = BACKLIGHT_CURRENT_CTRL_MODE;
	     setting->mode <= BACKLIGHT_TRIODE_MODE; setting->mode++) {
		for (setting->abms = 0; setting->abms <= 7; setting->abms++) {
			for (setting->abr = 0; setting->abr <= 3;
			     setting->abr++) {
				if (run_backlight_test(fp, setting) < 0) {
					printf("Test FAILED");
					return -1;
				}
			}
		}
	}

	/* Clean configuration */
	disable_backlight(fp, setting);

	printf("Test PASSED\n");
	return 0;
}

/*!
 * This is the suite test for the fun pattern.
 * It checks that the read/write operation are coherent.
 * Tester has to check the pattern
 *
 * @param        fp             the file pointer
 * @param        tcled_setting  the tcled setting
 * @param        fun_param        the fun parameter
 *
 * @return       This function returns 0 if successful.
 */
int fun_light_test(int fp,
		   t_tcled_enable_param * tcled_setting,
		   t_fun_param * fun_param)
{
	for (tcled_setting->bank = 0;
	     tcled_setting->bank <= 2; tcled_setting->bank++) {
		ioctl(fp, PMIC_TCLED_DISABLE, tcled_setting->bank);
	}

	printf("Read/Write Tests\n");
	tcled_setting->mode = TCLED_FUN_MODE;
	for (fun_param->bank = 0; fun_param->bank <= 2; fun_param->bank++) {
		for (fun_param->pattern = 0;
		     fun_param->pattern <= 11; fun_param->pattern++) {
			tcled_setting->bank = fun_param->bank;
			if (ioctl(fp, PMIC_TCLED_PATTERN, fun_param) &&
			    (fun_param->pattern != STROBE_SLOW) &&
			    (fun_param->pattern != STROBE_FAST)) {
				printf("Test FAILED");
				return -1;
			}
			ioctl(fp, PMIC_TCLED_ENABLE, tcled_setting);
			sleep(1);
			ioctl(fp, PMIC_TCLED_DISABLE, tcled_setting->bank);
		}
	}

	printf("Run different pattern\n");
	fun_param->bank = 0;
	tcled_setting->bank = 0;
	fun_param->pattern = 0;
	ioctl(fp, PMIC_TCLED_PATTERN, fun_param);
	ioctl(fp, PMIC_TCLED_ENABLE, tcled_setting);
	sleep(1);
	fun_param->bank = 1;
	tcled_setting->bank = 1;
	fun_param->pattern = 3;
	ioctl(fp, PMIC_TCLED_PATTERN, fun_param);
	ioctl(fp, PMIC_TCLED_ENABLE, tcled_setting);
	sleep(1);
	fun_param->bank = 2;
	tcled_setting->bank = 2;
	fun_param->pattern = 4;
	ioctl(fp, PMIC_TCLED_PATTERN, fun_param);
	ioctl(fp, PMIC_TCLED_ENABLE, tcled_setting);
	sleep(5);
	for (tcled_setting->bank = 0;
	     tcled_setting->bank <= 2; tcled_setting->bank++) {
		ioctl(fp, PMIC_TCLED_DISABLE, tcled_setting->bank);
	}

	printf("Run the same pattern on the three banks\n");
	for (fun_param->pattern = 0; fun_param->pattern <= 11;
	     fun_param->pattern++) {
		ioctl(fp, PMIC_TCLED_PATTERN, fun_param);
		for (fun_param->bank = 0; fun_param->bank <= 2;
		     fun_param->bank++) {
			tcled_setting->bank = fun_param->bank;
			ioctl(fp, PMIC_TCLED_ENABLE, tcled_setting);
		}
		sleep(3);
		for (fun_param->bank = 0; fun_param->bank <= 2;
		     fun_param->bank++) {
			tcled_setting->bank = fun_param->bank;
			ioctl(fp, PMIC_TCLED_DISABLE, tcled_setting->bank);
		}
	}

	printf("Test PASSED\n");
	return 0;
}

/*!
 * This is the unit test for the tcled ind pattern.
 * It checks that the read/write operation are coherent.
 *
 * @param        fp             the file pointer
 * @param        tcled_setting  the tcled setting
 * @param        ind_param      the tcled ind parameter
 *
 * @return       This function returns 0 if successful.
 */
int run_ind_light_test(int fp,
		       t_tcled_enable_param * tcled_setting,
		       t_tcled_ind_param * ind_param)
{
	t_tcled_ind_param r_ind_param;

	r_ind_param.channel = ind_param->channel;
	r_ind_param.bank = ind_param->bank;
	ioctl(fp, PMIC_SET_TCLED, ind_param);
	ioctl(fp, PMIC_GET_TCLED, &r_ind_param);
	ioctl(fp, PMIC_TCLED_ENABLE, tcled_setting);
	usleep(50);
	ioctl(fp, PMIC_TCLED_DISABLE, tcled_setting->bank);
	if ((ind_param->bank != r_ind_param.bank) ||
	    (ind_param->channel != r_ind_param.channel) ||
	    (ind_param->level != r_ind_param.level) ||
	    (ind_param->pattern != r_ind_param.pattern) ||
	    (ind_param->rampup != r_ind_param.rampup) ||
	    (ind_param->rampdown != r_ind_param.rampdown)) {
		printf("channel : %d - %d\n", r_ind_param.channel,
		       ind_param->channel);
		printf("level : %d - %d\n", r_ind_param.level,
		       ind_param->level);
		printf("pattern : %d - %d\n", r_ind_param.pattern,
		       ind_param->pattern);
		printf("rampup : %d - %d\n", r_ind_param.rampup,
		       ind_param->rampup);
		printf("rampdown : %d - %d\n", r_ind_param.rampdown,
		       ind_param->rampdown);
		return -1;
	}
	return 0;
}

/*!
 * It reset int_param struct
 *
 * @param        fp             the file pointer
 * @param        tcled_setting  the tcled setting
 * @param        ind_param      the tcled ind parameter
 *
 * @return       This function returns 0 if successful.
 */
void ind_reset(t_tcled_ind_param * ind_param)
{
	ind_param->skip = 0;
	ind_param->half_current = 0;
	ind_param->level = 0;
	ind_param->channel = 0;
	ind_param->pattern = 0;
	ind_param->rampup = 0;
	ind_param->rampdown = 0;
}

int single_ind_light_test(int fp, int bank, int color)
{
	t_tcled_enable_param tcled_setting;
	t_tcled_ind_param ind_param;

	ind_reset(&ind_param);

	tcled_setting.bank = bank;
	tcled_setting.mode = TCLED_IND_MODE;
	ind_param.bank = tcled_setting.bank;

	ind_param.channel = color;
	ind_param.level = TCLED_CUR_LEVEL_4;
	ind_param.pattern = TCLED_IND_BLINK_8;

	run_ind_light_test(fp, &tcled_setting, &ind_param);
	sleep(1);

	ind_param.level = TCLED_CUR_LEVEL_1;
	ind_param.pattern = TCLED_IND_OFF;
	run_ind_light_test(fp, &tcled_setting, &ind_param);

	printf("Test Passed\n");

	return 0;
}

/*!
 * This is the suite test for the tcled ind pattern.
 * It checks that the read/write operation are coherent.
 *
 * @param        fp             the file pointer
 * @param        tcled_setting  the tcled setting
 * @param        ind_param        the tcled ind parameter
 *
 * @return       This function returns 0 if successful.
 */
int ind_light_test(int fp,
		   t_tcled_enable_param * tcled_setting,
		   t_tcled_ind_param * ind_param)
{

	printf("Read/Write Tests\n");
	ind_reset(ind_param);
	tcled_setting->mode = TCLED_IND_MODE;
	for (tcled_setting->bank = 0;
	     tcled_setting->bank <= 2; tcled_setting->bank++) {
		ind_param->bank = tcled_setting->bank;
		for (ind_param->channel = TCLED_IND_RED;
		     ind_param->channel <= TCLED_IND_BLUE;
		     ind_param->channel++) {
			ind_param->level = TCLED_CUR_LEVEL_4;
			ind_param->pattern = TCLED_IND_BLINK_8;
			run_ind_light_test(fp, tcled_setting, ind_param);
			usleep(250);
			ind_param->level = TCLED_CUR_LEVEL_1;
			ind_param->pattern = TCLED_IND_OFF;
			run_ind_light_test(fp, tcled_setting, ind_param);
		}
	}
	ind_reset(ind_param);
	for (tcled_setting->bank = 0;
	     tcled_setting->bank <= 2; tcled_setting->bank++) {
		ind_param->bank = tcled_setting->bank;
		for (ind_param->channel = TCLED_IND_RED;
		     ind_param->channel <= TCLED_IND_BLUE;
		     ind_param->channel++) {
			for (ind_param->level = 0;
			     ind_param->level <= TCLED_CUR_LEVEL_4;
			     ind_param->level++) {
				for (ind_param->pattern = 0;
				     ind_param->pattern <= TCLED_IND_ON;
				     ind_param->pattern++) {
					if (run_ind_light_test(fp,
							       tcled_setting,
							       ind_param) < 0) {
						printf("Test FAILED");
						return -1;
					}
					usleep(250);
				}
			}
			ind_param->level = TCLED_CUR_LEVEL_1;
			ind_param->pattern = TCLED_IND_OFF;
			run_ind_light_test(fp, tcled_setting, ind_param);
		}
	}
	ind_reset(ind_param);

	printf("Test PASSED\n");
	return 0;
}

int main(int argc, char **argv)
{
	int fp;
	int tflag, bflag, cflag, pflag;
	char *ch_test_case, *ch_bank, *ch_color, *ch_pattern;
	int test_case, bank, color, pattern;
	char *msg = 0;
	t_bklit_setting_param setting;
	t_tcled_enable_param tcled_setting;
	t_tcled_ind_param ind_param;
	t_fun_param fun_param;

	option_t options[] = {
		{"T:", &tflag, &ch_test_case},	/* argument required */
		{"B:", &bflag, &ch_bank},
		{"F:", &pflag, &ch_pattern},
		{"C:", &cflag, &ch_color},
		{NULL, NULL, NULL}	/* NULL required to end array */
	};

	tflag = bflag = cflag = pflag = 0;
	ch_test_case = ch_bank = ch_color = ch_pattern = 0;
	test_case = bank = color = pattern = 0;

	if ((msg = parse_opts(argc, argv, options, &testcase_display)) != NULL) {
		printf("PMIC Light test did not work as expected.\n"
		       "Parse options error: %s\n", msg);
		testcase_display();
		return 1;
	}
	if (tflag == 0) {
		testcase_display();
		return 1;
	}

	test_case = atoi(ch_test_case);

	if (test_case < 1 || test_case > 8) {
		printf("Invalid arg for -T: %d\n"
		       "OPTION VALUE OUT OF RANGE\n", test_case);
		testcase_display();
		return 1;
	}

	if (test_case == 5) {
		if (bflag == 0) {
			bank_display();
			return 1;
		}
		bank = atoi(ch_bank);

		if (bank < 1 || bank > 3) {
			printf("Invalid arg for -B: %d\n", bank);
			bank_display();
			return 1;
		}

		if (pflag == 0) {
			printf("Invalid arg\n");
			pattern_display();
			return 1;
		}
		pattern = atoi(ch_pattern);

		if (pattern < 1 || pattern > 12) {
			printf("Invalid arg for -P: %d\n", pattern);
			pattern_display();
			return 1;
		}
	}

	if (test_case == 7) {
		if (bflag == 0) {
			bank_display();
			return 1;
		}
		bank = atoi(ch_bank);

		if (bank < 1 || bank > 3) {
			printf("Invalid arg for -B: %d\n", bank);
			bank_display();
			return 1;
		}

		if (cflag == 0) {
			color_display();
			return 1;
		}
		color = atoi(ch_color);

		if (color < 1 || color > 3) {
			printf("Invalid arg for -C: %d\n", color);
			color_display();
			return 1;
		}

	}

	fp = open("/dev/pmic_light", O_RDWR);

	if (fp < 0) {
		printf("Test Application: Open failed with error %d\n", fp);
		perror("Open");
		return -1;
	}

	ioctl(fp, PMIC_BKLIT_TCLED_ENABLE);

	switch (test_case) {
	case 1:
		printf("ENABLE BACKLIGHTS\n");
		enable_backlight(fp, &setting);
		printf("Check whether the BACKLIGHTS ON?\n");
		sleep(5);
		printf("DISABLE BACKLIGHTS\n");
		disable_backlight(fp, &setting);
		printf("Check whether the BACKLIGHTS OFF?\n");
		break;

	case 2:
		enable_backlight(fp, &setting);
		sleep(2);
		printf("Backlight should be enabled.\n");

		printf("Backlight channel 1 ramp up\n");
		ioctl(fp, PMIC_RAMPUP_BKLIT, BACKLIGHT_LED1);
		printf("Backlight channel 2 ramp up\n");
		ioctl(fp, PMIC_RAMPUP_BKLIT, BACKLIGHT_LED2);

		sleep(1);

		ioctl(fp, PMIC_OFF_RAMPUP_BKLIT, BACKLIGHT_LED1);
		ioctl(fp, PMIC_OFF_RAMPUP_BKLIT, BACKLIGHT_LED2);
		disable_backlight(fp, &setting);

		break;

	case 3:
		enable_backlight(fp, &setting);
		sleep(2);
		printf("Backlight should be enabled.\n");

		printf("Backlight channel 1 ramp down\n");
		ioctl(fp, PMIC_RAMPDOWN_BKLIT, BACKLIGHT_LED1);
		printf("Backlight channel 2 ramp down\n");
		ioctl(fp, PMIC_RAMPDOWN_BKLIT, BACKLIGHT_LED2);

		sleep(1);

		ioctl(fp, PMIC_OFF_RAMPDOWN_BKLIT, BACKLIGHT_LED1);
		ioctl(fp, PMIC_OFF_RAMPDOWN_BKLIT, BACKLIGHT_LED2);
		disable_backlight(fp, &setting);
		break;

	case 4:
		backlight_test(fp, &setting);
		break;

	case 5:
		tcled_setting.bank = bank - 1;
		fun_param.pattern = pattern - 1;
		start_fun_pattern(fp, &tcled_setting, &fun_param);
		break;

	case 6:
		fun_light_test(fp, &tcled_setting, &fun_param);
		break;

	case 7:
		single_ind_light_test(fp, bank - 1, color - 1);
		break;

	case 8:
		ind_light_test(fp, &tcled_setting, &ind_param);
		break;

	default:
		printf("Invalid Test case option\n");
		break;
	}

	ioctl(fp, PMIC_BKLIT_TCLED_DISABLE);

	close(fp);
	return 0;
}
