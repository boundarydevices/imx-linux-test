#!/bin/bash

source /unit_tests/test-utils.sh
#
#Save current directory
#
pushd .
#
#run modprobe test
#
#gpu_mod_name=galcore.ko
#modprobe_test $gpu_mod_name
#
#run tests
#
cd /opt/viv_samples/vdk/ && ./tutorial3 -f 100
cd /opt/viv_samples/vdk/ && ./tutorial4_es20 -f 100
cd /opt/viv_samples/tiger/ &&./tiger
echo press ESC to escape...
cd /opt/viv_samples/hal/ && ./tvui
#
#remove gpu modules
#
#rmmod $gpu_mod_name
#restore the directory
#
popd
