#!/bin/bash

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

# devnode test
check_devnode "/dev/pmic"

# protocol test cases
for CASE in SU OC CA; do
	run_testcase "./mc34704_testapp.out -T $CASE"
done

print_status
exit $STATUS
