#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

# devnode test
check_devnode "/dev/pmic"
check_devnode "/dev/pmic_adc"
check_devnode "/dev/pmic_battery"
check_devnode "/dev/pmic_light"
check_devnode "/dev/pmic_rtc"
#check_devnode "/dev/ts"

# protocol test cases
for CASE in RW SU OC CA; do
	run_testcase "./pmic_testapp -T $CASE"
done

# battery test cases
echo "not running pmic_testapp_battery -T 6 as it hangs, TODO FIX"
for CASE in 0 1 2 3 4 5; do
	run_testcase "./pmic_testapp_battery -T $CASE"
done

# RTC test cases
for CASE in TIME ALARM WAIT_ALARM TEST POLL_TEST; do
	run_testcase "./pmic_testapp_rtc -T $CASE"
done

# light test cases
for CASE in 1 2 3 4 6 8; do
	run_testcase "./pmic_testapp_light -T $CASE"
done
for BANK in 1 2 3; do
	for COLOR in 1 2 3; do
		run_testcase "./pmic_testapp_light -T 7 -B $BANK -C $COLOR"
	done		
done
for BANK in 1 2 3; do
	for PATTERN in 1 2 3 4 5 6 7 8 9 10 11 12; do
		run_testcase "./pmic_testapp_light -T 5 -B $BANK -F $PATTERN"
	done		
done

# power test cases
# insmod test
insmod_test /lib/modules/$(kernel_version)/test/mxc_pmic_power_testmod.ko
# devnode test
check_devnode "/dev/pmic_power"

if [ "$STATUS" = 0 ]; then
	for CASE in 1 2 3; do
		run_testcase "./pmic_testapp_power -T $CASE"
	done
fi

print_status
exit $STATUS
