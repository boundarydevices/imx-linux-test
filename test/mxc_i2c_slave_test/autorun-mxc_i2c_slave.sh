#!/bin/bash

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

check_devnode "/dev/slave-i2c-0"
run_testcase "./mxc_i2c_slave_test.out"

print_status
exit $STATUS

