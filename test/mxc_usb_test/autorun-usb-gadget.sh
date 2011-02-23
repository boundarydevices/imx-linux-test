#!/bin/bash

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0
DESCRIPTION="USB Ethernet Gadget test"
# Modules in the order in which they should be loaded
MODULES="/lib/modules/$(kernel_version)/kernel/drivers/usb/gadget/arcotg_udc.ko
         /lib/modules/$(kernel_version)/kernel/drivers/usb/gadget/g_ether.ko"

# IP address of the USB Host PC to which the target is connected to
USB_HOST_IP=$1
USB_TARGET_IP="192.168.0.2"
USB_IFACE="usb0"
PACKETS=4
SIZE=512
PING_CMD="ping -c $PACKETS -s $SIZE $USB_HOST_IP"
PING_LOG="/root/ping_time.log"
PING_LOG_1="/root/ping_time_1.log"
# PING_CMD="ping -f -q -u 100 -c 10000 -s 512 $USB_HOST_IP"

script=$(basename $0)
echo $DESCRIPTION

rm -f $PING_LOG
rm -f $PING_LOG_1

if [ -z $1 ]; then
	echo "Usage: $script <IP address of host PC>"
	STATUS=1
	print_status
	exit $STATUS;
fi

for i in $MODULES; do
	insmod_test $i
	if [ "$STATUS" != 0 ]; then
		print_status
	    	exit $STATUS
	fi
done

/sbin/ifconfig $USB_IFACE $USB_TARGET_IP

# Wait for USB host PC to configure its IP address
sleep 2

{ time $PING_CMD; } 1>$PING_LOG_1 2>$PING_LOG
if [ "$?" == 0 ] ; then
	TIME=`cat $PING_LOG | grep real | awk '{ printf "%f\n", $2 * 60 + $3 }';`
	PACKETS=`cat $PING_LOG_1 | grep transmitted | awk '{ printf "%d\n", ($1 + $4) }';`
	SIZE=`cat $PING_LOG_1 | grep PING | awk '{ printf "%d\n", $4 }';`
	DATA_RATE=`echo $PACKETS $SIZE $TIME | awk '{ printf "%f\n", $1 * $2 * 8/ ($3 * 1000000) }';`

	echo "USB <$TIME sec> <$PACKETS packets> <$SIZE bytes> <$DATA_RATE Mbits/s>"
else
	STATUS=1
fi

rmmod g_ether
rmmod arcotg_udc

print_status
exit $STATUS
