#!/bin/bash

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0
MASTER_PATH="/sys/devices/w1_bus_master1"
script=$(basename $0)

# Test message is small (<16 bytes) to accomodate small eeprom size of DS2751 battery fuel gauge
# which is on some boards.
MESSAGE="$(date '+%H:%M:%S')"

if [ "$(platform)" = IMX27ADS ]; then
	# This check allows the test to be run multiple times without getting errors
	if [ ! -e $MASTER_PATH ]; then
		insmod_test /lib/modules/$(kernel_version)/kernel/drivers/w1/wire.ko
		insmod_test /lib/modules/$(kernel_version)/kernel/drivers/w1/masters/mxc_w1.ko
		insmod_test /lib/modules/$(kernel_version)/kernel/drivers/w1/slaves/w1_ds2433.ko
	fi
fi

sleep 2

echo
echo "Part 1: Checking for the one wire master"
if [ ! -e $MASTER_PATH ]; then
	STATUS=1
	echo "$script: FAIL One wire Master Not Found $MASTER_PATH"
	echo "$script:      One wire does not appear to be working on this board."
	echo
	print_status
	exit $STATUS
else
	echo "$script: PASS One Wire Master Found $MASTER_PATH"
	echo
	echo "ls -aF $MASTER_PATH"
	ls -aF $MASTER_PATH
	echo
fi

echo "Part 2: Checking for Slave device"
# Each DS2433 EEPROM has a unique id number which starts with 'family code' of '23-' or '09-'
SLAVE_COUNT="$(cat /sys/devices/w1_bus_master1/w1_master_slave_count)"
# Not sure how this will work if there is more than one device on the platform.
SLAVE_PATH="/sys/devices/w1_bus_master1/$(cat /sys/devices/w1_bus_master1/w1_master_slaves|grep '[0-9]*')"

echo "w1_master_slave_count: $SLAVE_COUNT"
echo "w1_master_slaves: $(cat /sys/devices/w1_bus_master1/w1_master_slaves)"

if [ "$SLAVE_COUNT" = 0 ]; then
        STATUS=1
        echo -e "$script: CHECK MANUALLY: One wire slave count is 0"
        echo -e "$script:        No one-wire slaves devices found on this board."
        echo -e "$script:        Check whether there should be.\n"
	print_status
	exit $STATUS
elif cat /sys/devices/w1_bus_master1/w1_master_slaves|grep -sq 'not found'; then
        STATUS=1
        echo -e "$script: FAIL w1_master_slaves says \"not found\""
        echo -e "$script:      Since there is at least one device, it should have been found.\n"
	print_status
	exit $STATUS
elif [ ! -e $SLAVE_PATH ]; then
        STATUS=1
        echo -e "$script: FAIL One wire Slave Not Found at $SLAVE_PATH"
        echo -e "$script:      Since there is at least one device, it should have been found.\n"
	print_status
	exit $STATUS
fi

echo -e "$script: PASS One Wire Slave Found $SLAVE_PATH\n"
echo "ls -aF $SLAVE_PATH"
ls -aF $SLAVE_PATH
echo

echo "Part 3: Reading from Slave device"
if [ -e $SLAVE_PATH/id ]; then
	read_back="$(cat $SLAVE_PATH/id)"
	echo "Value got from id ...$read_back"
	echo -e "$script: PASS One Wire Slave read id\n"
elif [ -e $SLAVE_PATH/eeprom ]; then
	read_back="$(cat $SLAVE_PATH/eeprom)"
	echo "Value got from eeprom ...$read_back"
	echo -e "$script: PASS One Wire Slave read eeprom\n"
else
	STATUS=1
	echo -e "$script: CHECK MANUALLY No readable item recgnized by this script."
	echo -e "$script:            There should be one, but it may have a different name.\n"
	print_status
	exit $STATUS
fi

echo "Part 4: Writing to Slave device"
if [ -e $SLAVE_PATH/eeprom ]; then
	if [ ! -w $SLAVE_PATH/eeprom ]; then
		STATUS=1
		echo "$script: FAIL One wire Slave eeprom file Not writable"
		echo "ls -lh $SLAVE_PATH/eeprom"
		ls -lh $SLAVE_PATH/eeprom
	else
		echo "Writing \"$MESSAGE\" to eeprom"
		echo "$MESSAGE" > $SLAVE_PATH/eeprom
		echo "Try Reading EEPROM"
		read_back="$(cat $SLAVE_PATH/eeprom | grep "$MESSAGE")"
		echo "Value got from eeprom ...$read_back"
		if [ "$read_back" = "$MESSAGE" ]; then
			echo "$script: PASS EEPROM Read Correctly"
		else
			STATUS=1
			echo "String read from eeprom was:"
			echo "$read_back"
			echo "$script: FAIL To Read EEPROM"
                fi
	fi
else
	echo -e "$script: CHECK MANUALLY This does not appear to be a writable one wire device."
	echo -e "$script:                If it should be, then this test failed.\n"
fi

echo
print_status
exit $STATUS
