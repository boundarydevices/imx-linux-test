#!/bin/bash

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

uart_list="0 1 2" ;

for i in $uart_list; do
	check_devnode "/dev/ttymxc${i}"
done

for i in $uart_list; do
	run_testcase "./mxc_uart_test.out /dev/ttymxc${i}"
done

print_status
exit $STATUS

