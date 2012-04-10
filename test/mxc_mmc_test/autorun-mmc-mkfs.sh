#!/bin/bash

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

run_mmc_case()
{
	mkfs.ext2 /dev/mmcblk0p1 && mkfs.minix /dev/mmcblk0p1

	if [ "$?" = 0 ]; then
		printf "MMC test passes \n\n"
	else
		STATUS=1
		printf "MMC test fails \n\n"
	fi
}

# devnode test
check_devnode "/dev/mmcblk0p1"

if [ "$STATUS" = 0 ]; then
	run_mmc_case
fi

print_status
exit $STATUS
