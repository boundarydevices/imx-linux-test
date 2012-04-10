#!/bin/bash

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

run_mmc_case()
{
	# create two partitions
	fdisk /dev/mmcblk0 2>&1 1>/dev/null  << EOF
	p
	d
	1
	d
	2
	d
	3
	d
	n
	p
	1

	+10M
	n
	p
	2

	+20M

	w
EOF
	sleep 1
	if [ "$?" = 0 ]; then
		if [ -e '/dev/mmcblk0p1' ] && [ -e '/dev/mmcblk0p2' ]; then
			printf "MMC test passes \n\n"
			return
		fi
	fi
	STATUS=1
	printf "MMC test fails \n\n"
}

# devnode test
check_devnode "/dev/mmcblk0"

if [ "$STATUS" = 0 ]; then
	run_mmc_case
fi

print_status
exit $STATUS
