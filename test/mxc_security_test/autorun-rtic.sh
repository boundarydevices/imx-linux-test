#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

# Inserting the RNG Module.
insmod_test /lib/modules/$(kernel_version)/test/mxc_rtic_test.ko

# protocol test cases
run_testcase "./mxc_security_test.out 2 1 0 2 0 3 4 0 6 8 9"

print_status
exit $STATUS

