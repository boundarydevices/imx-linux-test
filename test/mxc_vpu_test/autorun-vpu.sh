#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

if [ "$(platform)" = IMX31ADS ]; then
	exit $STATUS
fi

# devnode test
check_devnode "/dev/mxc_vpu"

run_testcase "./mxc_vpu_test.out -C config_dec"

if [ "$(platform)" = IMX27ADS ]; then
run_testcase "./mxc_vpu_test.out -C config_enc"
rm -f test.264
fi

print_status
exit $STATUS
