#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

oprofile_test()
{
	echo "Running OProfile Test:"
	opcontrol --no-vmlinux
	opcontrol --init
	opcontrol --start
	opcontrol --dump
	sleep 1
	opcontrol --dump
	opreport > /tmp/tmp_file
	opcontrol --stop
	opcontrol --shutdown

	# devnode test
	check_devnode "/dev/oprofile"

	output=`cat /tmp/tmp_file | wc -l`      
	if test $output -ge 10 ; then 
		printf "OProfile Test Passed \n\n"
		rm /tmp/tmp_file
	else
		STATUS=1
		printf "OProfile Test Failed \n\n"
	fi         
	# sleep a little to let autotest.pl script catch up with the logging.
	sleep 2
}

cmd=`which opcontrol`
check_executable "$cmd"

if [ "$STATUS" = 0 ]; then 
	oprofile_test
fi

print_status
exit $STATUS
