#!/bin/bash

source /unit_tests/test-utils.sh

STATUS=0
if [[ $(platform) != i.MX6Q* ]] && [[ $(platform) != i.MX6D* ]] \
&& [[ $(platform) != i.MX6SX ]] && [[ $(platform) != i.MX6SL ]]; then
	echo "gpuinfo.sh not supported on current soc"
	exit $STATUS
fi

input=$1
echo "GPU Info"
cat /sys/kernel/debug/gc/info
cat /sys/kernel/debug/gc/meminfo
echo "Paged memory Info"
cat /sys/kernel/debug/gc/allocators/default/lowHighUsage
echo "CMA memory info"
cat /sys/kernel/debug/gc/allocators/cma/cmausage
#echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
time=`cat /sys/kernel/debug/gc/idle`
#echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
if [ x$input != x ]
then
pidlist=`cat /sys/kernel/debug/gc/clients | awk '{if ($1 == '$input') print $1}'`
if [ x$pidlist == x ]
then
pidlist=`cat /sys/kernel/debug/gc/clients | awk '{if ($2~/'$input'/) print $1}'`
fi
if [ x$pidlist == x ]
then
echo "Invalid input: $input"
fi
else
pidlist=`cat /sys/kernel/debug/gc/clients | awk '{if ($1~/^[0-9]/) print $1}'`
fi
for i in $pidlist
do
    echo $i > /sys/kernel/debug/gc/vidmem
    cat /sys/kernel/debug/gc/vidmem
done
echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
sleep 1
time=`cat /sys/kernel/debug/gc/idle | awk '{printf "%.2f", $7/10000000.0}'`
echo "Idle percentage:"$time"%"
echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
