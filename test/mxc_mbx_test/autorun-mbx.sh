#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0
script=$(basename $0)

run_testcase "/etc/rc.d/init.d/rc.pvr start"
device_1=$(cat /proc/devices | grep clcdc | cut -d' ' -f2)
device_2=$(cat /proc/devices | grep swcamera | cut -d' ' -f2)
echo device present is $device_1
echo device present is $device_2

if [ "$device_1" = clcdc ]; then
	echo "$script: PASS Device Found $device_1"
else
	STATUS=1
	echo "$script: FAIL Device Not Found $device_1"
	echo
	print_status
	exit $STATUS
fi

if [ "$device_2" = swcamera ]; then
	echo "$script : PASS Device Found $device_2"
else
	STATUS=1
	echo "$script: FAIL Device Not Found $device_2"
	echo
	print_status
	exit $STATUS
fi

echo "For this test case you will see rotating Triangle on LCD"
run_testcase "/usr/local/bin/egl_test 1000"
sleep 4

echo "For this test case you will see a Colouring Pattern on LCD"
run_testcase "/usr/local/bin/services_test"
sleep 2

run_testcase "/etc/rc.d/init.d/rc.pvr stop"

print_status
exit $STATUS

