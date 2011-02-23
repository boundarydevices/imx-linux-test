#!/bin/bash

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

# Inserting the sahara Module.
insmod_test /lib/modules/$(kernel_version)/test/sahara_test_driver.ko

mknod /dev/sahara c `grep sahara /proc/devices | sed -e 's/ .*//'` 0

# devnode test
check_devnode "/dev/sahara"

# protocol test cases
for CASE in F hg mg rg sg acg; do
        run_testcase "./apitest -T $CASE"
done

print_status
exit $STATUS
