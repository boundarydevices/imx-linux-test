#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

if [ "$(platform)" = IMX32ADS ]; then
	exit $STATUS
fi

# devnode test
check_devnode "/dev/hmp4e"
insmod_test /lib/modules/$(kernel_version)/test/memalloc.ko

run_testcase "./cam2mpeg4.out 352 288 50 30 test.mp4"
rm -f test.mp4

print_status
exit $STATUS
