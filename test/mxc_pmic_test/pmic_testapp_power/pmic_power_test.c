#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>		/* ioctl() */
#include <unistd.h>		/* close() */
#include <string.h>

#include "test.h"
#include "usctest.h"

#include <asm/arch/pmic_power.h>
#include <asm/arch/pmic_status.h>
#include <asm/arch/pmic_external.h>

#ifdef CONFIG_MXC_PMIC_SC55112

#define REGU_START	SW1
#define REGU_END	(VMMC + 1)
#define REGU_VIBRATOR	REGU_END
#define POWER_CONFIG	"sc55112_power.cfg"

#elif CONFIG_MXC_PMIC_MC13783

#define REGU_START	SW_SW3
#define REGU_END	REGU_GPO1
#define REGU_VIBRATOR	REGU_VVIB
#define POWER_CONFIG	"mc13783_power.cfg"

#endif

/*==============================================================================
                                       GLOBAL VARIABLES
==============================================================================*/
int fp = 0;

int pmic_power_test_config(void)
{
	int ret;
	t_regulator_cfg_param set_cfg, get_cfg;
	FILE *file = fopen(POWER_CONFIG, "r");

	if(NULL == file) {
	    	printf("Could not open the config file %s\n",POWER_CONFIG);
		return -1;
	}

	while (!feof(file)) {
		memset(&set_cfg, 0, sizeof(t_regulator_cfg_param));
#ifdef CONFIG_MXC_PMIC_SC55112
		ret = fscanf(file, "%d %d %d %d %d %d\n",
			     (int *)&set_cfg.regulator,
			     (int *)&set_cfg.cfg.mode,
			     (int *)&set_cfg.cfg.voltage,
			     (int *)&set_cfg.cfg.voltage_lvs,
			     (int *)&set_cfg.cfg.voltage_stby,
			     (int *)&set_cfg.cfg.lp_mode);
#elif CONFIG_MXC_PMIC_MC13783
		ret = fscanf(file, "%d %d %d %d %d %d %d %d %d %d %d\n",
			     (int *)&set_cfg.regulator,
			     (int *)&set_cfg.cfg.mode,
			     (int *)&set_cfg.cfg.stby_mode,
			     (int *)&set_cfg.cfg.voltage,
			     (int *)&set_cfg.cfg.voltage_lvs,
			     (int *)&set_cfg.cfg.voltage_stby,
			     (int *)&set_cfg.cfg.lp_mode,
			     (int *)&set_cfg.cfg.dvs_speed,
			     (int *)&set_cfg.cfg.panic_mode,
			     (int *)&set_cfg.cfg.softstart,
			     (int *)&set_cfg.cfg.factor);
#endif

		if (ioctl(fp, PMIC_REGULATOR_SET_CONFIG, &set_cfg) !=
		    PMIC_SUCCESS) {
			printf("Test Set FAILED\n");
			fclose(file);
			return -1;
		}

		memset(&get_cfg, 0, sizeof(t_regulator_cfg_param));
		get_cfg.regulator = set_cfg.regulator;
		if (ioctl(fp, PMIC_REGULATOR_GET_CONFIG, &get_cfg) !=
		    PMIC_SUCCESS) {
			printf("Test Get FAILED\n");
			fclose(file);
			return -1;
		}
#ifdef CONFIG_MXC_PMIC_SC55112
		printf("Get Values%d %d %d %d %d %d\n",
		       get_cfg.regulator,
		       get_cfg.cfg.mode,
		       get_cfg.cfg.voltage,
		       get_cfg.cfg.voltage_lvs,
		       get_cfg.cfg.voltage_stby, get_cfg.cfg.lp_mode);
		if ((set_cfg.cfg.mode != get_cfg.cfg.mode) ||
		    (set_cfg.cfg.voltage.sw1 != get_cfg.cfg.voltage.sw1) ||
		    (set_cfg.cfg.voltage_lvs.sw1 != get_cfg.cfg.voltage_lvs.sw1)
		    || (set_cfg.cfg.voltage_stby.sw1 !=
			get_cfg.cfg.voltage_stby.sw1)
		    || (set_cfg.cfg.lp_mode != get_cfg.cfg.lp_mode)) {
			printf("Test Compare FAILED\n");
			fclose(file);
			return -1;
		}
#elif CONFIG_MXC_PMIC_MC13783
		if ((set_cfg.cfg.mode != get_cfg.cfg.mode) ||
		    (set_cfg.cfg.stby_mode != get_cfg.cfg.stby_mode) ||
		    (set_cfg.cfg.voltage.sw1a != get_cfg.cfg.voltage.sw1a) ||
		    (set_cfg.cfg.voltage_lvs.sw1a !=
		     get_cfg.cfg.voltage_lvs.sw1a)
		    || (set_cfg.cfg.voltage_stby.sw1a !=
			get_cfg.cfg.voltage_stby.sw1a)
		    || (set_cfg.cfg.lp_mode != get_cfg.cfg.lp_mode)
		    || (set_cfg.cfg.dvs_speed != get_cfg.cfg.dvs_speed)
		    || (set_cfg.cfg.panic_mode != get_cfg.cfg.panic_mode)
		    || (set_cfg.cfg.softstart != get_cfg.cfg.softstart)
		    || (set_cfg.cfg.factor != get_cfg.cfg.factor)) {
			printf("Test Compare FAILED\n");
			fclose(file);
			return -1;
		}
#endif
		printf("REGULATOR %d : Test PASSED\n", get_cfg.regulator);

	}

	fclose(file);
	return 0;
}

void testcase_display(void)
{
	printf("\n");
	printf("*******************************************************\n");
	printf("********** PMIC POWER Test options ********************\n");
	printf("*******************************************************\n");
	printf("\n");
	printf("*******************************************************\n");
	printf("***** '-T 1' Test Regulator ON and OFF ****************\n");
	printf("***** '-T 2' Test SWITCHES/REGULATOR Configuration ****\n");
	printf("***** '-T 3' Test Miscellaneos Features ***************\n");
	printf("*******************************************************\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	int i;
	int change_test_case = 0;
	int tflag = 0;
	char *ch_test_case = 0;
	char *msg = 0;

	option_t options[] = {
		{"T:", &tflag, &ch_test_case},	/* argument required */
		{NULL, NULL, NULL}	/* NULL required to end array */
	};

	if ((msg = parse_opts(argc, argv, options, &testcase_display)) != NULL) {
		printf("PMIC Power test did not work as expected.\n"
		       "Parse options error: %s\n", msg);
		testcase_display();
		return 1;
	}
	if (tflag == 0) {
		testcase_display();
		return 1;
	}

	change_test_case = atoi(ch_test_case);

	if (change_test_case < 1 || change_test_case > 3) {
		printf("Invalid arg for -T: %d\n"
		       "OPTION VALUE OUT OF RANGE\n", change_test_case);
		testcase_display();
		return 1;
	}

	fp = open("/dev/pmic_power", O_RDWR);

	if (fp < 0) {
		printf("Test module: Open failed with error %d\n", fp);
		perror("Open");
		return -1;
	}

	switch (change_test_case) {
	case 1:
		printf("Testing if REGULATOR ON is OK\n");
		for (i = REGU_START; i < REGU_END; i++) {
			if (ioctl(fp, PMIC_REGULATOR_ON, i) != PMIC_SUCCESS) {
				printf("Test FAILED\n");
			}
			printf("REGULATOR %d : ON\n", i);
		}
		printf("Test PASSED\n");
		sleep(1);
		printf("Testing if REGULATOR OFF is OK\n");
		printf("Switching OFF is restricted to limited of Regulator\n"
		       "as its' dangerous to switch-off regulators randomly.\n");
		for (i = REGU_START; i < REGU_END; i++) {
#ifdef CONFIG_MXC_PMIC_SC55112
#elif CONFIG_MXC_PMIC_MC13783
			if (i != SW_SW3 && i != SW_PLL && i != REGU_VAUDIO
			    && i != REGU_VCAM && i != REGU_VVIB) {
				continue;
			}
#endif
			if (ioctl(fp, PMIC_REGULATOR_OFF, i) != PMIC_SUCCESS) {
				printf("Test FAILED\n");
			}
			printf("REGULATOR %d : OFF\n", i);
		}
		printf("Test PASSED\n");
		break;
	case 2:
		printf("Testing if SWITCHES/REGULATOR configurations are OK\n");
		for (i = REGU_START; i < REGU_END; i++) {
			if (i == REGU_VIBRATOR)
				continue;
			if (ioctl(fp, PMIC_REGULATOR_ON, i) != PMIC_SUCCESS) {
				printf("Test FAILED\n");
			}
		}
		if (pmic_power_test_config() != PMIC_SUCCESS) {
			printf("TEST CASE FAILED\n");
		} else {
			printf("TEST CASE PASSED\n");
		}
		break;

	case 3:
		printf("Testing if Miscellaneous features are OK\n");
		printf("This testcase is not valid for SC55112\n");
		if (ioctl(fp, PMIC_POWER_CHECK_MISC, NULL) != PMIC_SUCCESS) {
			printf("TEST CASE FAILED\n");
		} else {
			printf("TEST CASE PASSED\n");
		}
		break;
	}

	close(fp);
	return 0;
}
