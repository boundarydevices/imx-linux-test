#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

# Inserting the RNG Module.
insmod_test /lib/modules/$(kernel_version)/test/mxc_HAC_test.ko

# protocol test cases
run_testcase "./mxc_security_test.out 1 1 2 3"

print_status
exit $STATUS

