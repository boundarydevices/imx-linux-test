#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0
platform=`cat /proc/cpuinfo | grep Hardware |cut -f3 -d' '`
 if  [ $platform = "MXC300-31" ]  || [ $platform = "MXC300-20"  ]
 then 
 m2ramaddr=10000100
 else
 m2ramaddr=ff002800
fi
#echo M2RAM Address is: $m2ramaddr
# MU protocol test case
echo $m2ramaddr | run_testcase "./mxc_mu_test.out "

print_status
exit $STATUS
