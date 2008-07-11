#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

# devnode test
check_devnode "/dev/pmic"
check_devnode "/dev/pmic_rtc"

# protocol test cases
run_testcase "./mx35_pmic_testapp"

# RTC test cases
for CASE in TIME ALARM WAIT_ALARM TEST POLL_TEST; do
	run_testcase "./mx35_pmic_testapp_rtc -T $CASE"
done

