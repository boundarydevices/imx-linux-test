#!/bin/bash

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

case $(platform) in
	IMX31ADS | IMX32ADS )	uart_list="0 2 4" ;;
	MXC91131EVB)		uart_list="0 2";;
	* )			uart_list="0 1 2" ;;
esac

for i in $uart_list; do
	check_devnode "/dev/ttymxc${i}"
done

for i in $uart_list; do
	run_testcase "./mxc_uart_test.out /dev/ttymxc${i}"
done

print_status
exit $STATUS

