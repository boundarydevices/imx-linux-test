#!/bin/bash

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

run_mmc_case()
{
	if [ ! -d /mnt/mmc_part1 ]; then
		mkdir /mnt/mmc_part1
	fi
	mounted=`mount | grep '/dev/mmcblk0p1' | wc -l`
	if [ $mounted = 1 ]; then
		umount /dev/mmcblk0p1
	fi
	mounted=`mount | grep '/mnt/mmc_part1' | wc -l`
	if [ $mounted = 1 ]; then
		umount /mnt/mmc_part1
	fi
	mkfs.ext2 /dev/mmcblk0p1
	mount -t ext2 /dev/mmcblk0p1 /mnt/mmc_part1

	dd if=/dev/urandom of=/root/mmc_data bs=1M count=5
	cp /root/mmc_data /mnt/mmc_part1/mmc_data
	sync

	cmp /root/mmc_data /mnt/mmc_part1/mmc_data

	if [ "$?" = 0 ]; then
		printf "MMC test passes \n\n"
		rm /root/mmc_data
		rm /mnt/mmc_part1/mmc_data
		umount /mnt/mmc_part1
		rm -r /mnt/mmc_part1
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
