#!/bin/sh

#   On imx31ads board, turn off buzzer (set SW2-2 to OFF).

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

run_hdd_case()
{
	# Generate Test data
	dd if=/dev/urandom of=/root/hdd_data bs=512 count=10240

	if [ "$(platform)" = "IMX31_3STACK" ]
	then
		dd if=/root/hdd_data of=/dev/hda bs=512 count=10240
		dd if=/dev/hda of=/root/hdd_data1 bs=512 count=10240
	else
	
		dd if=/root/hdd_data of=/dev/sda bs=512 count=10240
		dd if=/dev/sda of=/root/hdd_data1 bs=512 count=10240
	fi

	cmp /root/hdd_data1 /root/hdd_data

	if [ "$?" = 0 ]; then
		printf "HDD test passes \n\n"
		rm /root/hdd_data
		rm /root/hdd_data1
	else
		STATUS=1
		printf "HDD test fails \n\n"
	fi
}

#setup
if [ "$(platform)" = "IMX27ADS" ]
then
	echo "IMX27ADS"
fi
if [ "$(platform)" = "IMX31_3STACK" ]
then
	echo "IMX31_3STACK"
	modprobe mxc-ide
	sleep 5
	modprobe ide_disk
	sleep 1
else
	echo "IMX35_3STACK"
	modprobe pata_fsl
	sleep 5
fi

# devnode test
if [ "$(platform)" = "IMX31_3STACK" ]
then
	check_devnode "/dev/hda"
else
	check_devnode "/dev/sda"
fi

if [ "$STATUS" = 0 ]; then
	run_hdd_case
fi

print_status
exit $STATUS
