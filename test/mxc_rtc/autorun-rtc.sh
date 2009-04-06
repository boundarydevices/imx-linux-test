#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

echo "$(platform)"
if [ "$(platform)" == IMX37_3STACK ] || [ "$(platform)" == IMX51 ]; then
	rtc_test_param=--no-periodic
	RTC_IRQS_EXPECTED=1
else
	rtc_test_param=--full
	RTC_IRQS_EXPECTED=131
fi

# devnode test
check_devnode "/dev/rtc0"

# check rtc interrupts
RTC_IRQS_BEFORE=$( cat /proc/interrupts |grep rtc|sed -r 's,.*: *([0-9]*) .*,\1,' )

# RTC test cases
run_testcase "./rtctest.out $rtc_test_param"

RTC_IRQS_AFTER=$( cat /proc/interrupts |grep rtc|sed -r 's,.*: *([0-9]*) .*,\1,' )

RTC_IRQS=$(( $RTC_IRQS_AFTER - $RTC_IRQS_BEFORE ))

echo "rtc irqs before running unit test: $RTC_IRQS_BEFORE"
echo "rtc irqs after running unit test:  $RTC_IRQS_AFTER"
echo "so rtc irqs during test was:       $RTC_IRQS"

if [ "$RTC_IRQS" != "$RTC_IRQS_EXPECTED" ]; then
	echo "checking rtc interrupts FAIL, expected $RTC_IRQS_EXPECTED interrupts, got $RTC_IRQS"
	STATUS=1
else
	echo "checking rtc interrupts PASS"
fi

print_status
exit $STATUS
