#!/bin/sh

source /unit_tests/test-utils.sh

mount_test()
{
	if ! mount $MSC_DEVICE $MSC_DIR; then
		STATUS=1
		echo "mount failed."
		print_status
		exit $STATUS
	fi
}

umount_test()
{
	if ! umount $MSC_DIR; then
		STATUS=1
		echo "umount failed."
		print_status
		exit $STATUS
	fi
}

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0
DESCRIPTION="USB Ethernet Host test"

MSC_DEVICE=/dev/sda1
MSC_DIR=/mnt/usb

script=$(basename $0)
echo $DESCRIPTION

insmod_test /lib/modules/$(kernel_version)/kernel/drivers/usb/host/ehci-hcd.ko
if [ "$STATUS" != 0 ]; then
	print_status
	exit $STATUS
fi

sleep 8

cat <<EOF > /root/testfile1
The quick brown fox jumps over the lazy dog
The quick brown fox jumps over the lazy dog
The quick brown fox jumps over the lazy dog
The quick brown fox jumps over the lazy dog
The quick brown fox jumps over the lazy dog
The quick brown fox jumps over the lazy dog
EOF

mkdir -p $MSC_DIR

mount_test
cp /root/testfile1 $MSC_DIR/testfile2
umount_test

mount_test
cp $MSC_DIR/testfile2 /root/testfile3
rm $MSC_DIR/testfile2
umount_test

if ! cmp /root/testfile1 /root/testfile3 >/dev/null 2>/dev/null; then
	STATUS=1
	echo "Compare Failed"
fi

rm /root/testfile1
rm /root/testfile3

rmmod ehci-hcd

print_status
exit $STATUS
