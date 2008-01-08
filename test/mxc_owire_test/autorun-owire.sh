#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0
script=$(basename $0)

# Test message is small (<16 bytes) to accomodate small eeprom size of DS2751 battery fuel gauge
# which is on the mxc30031ads boards.
MESSAGE="$(date '+%H:%M:%S')"

if [ "$(platform)" = IMX27ADS ]; then
	insmod_test /lib/modules/$(kernel_version)/kernel/drivers/w1/wire.ko
	insmod_test /lib/modules/$(kernel_version)/kernel/drivers/w1/masters/mxc_w1.ko
	insmod_test /lib/modules/$(kernel_version)/kernel/drivers/w1/slaves/w1_ds2433.ko
fi

sleep 2

echo "Checking for One wire master"
MASTER_PATH="/sys/devices/w1_bus_master1"
if [ ! -e $MASTER_PATH ]; then
	STATUS=1
	echo "$script: FAIL One wire Master Not Found $MASTER_PATH"
	echo
	print_status
	exit $STATUS
else
	echo "$script: PASS One Wire Master Found $MASTER_PATH"
	echo
	echo "ls $MASTER_PATH"
	ls $MASTER_PATH
	echo
fi

echo "Checking for Slave device"
# Each DS2433 EEPROM has a unique id number which starts with 'family code' of '23-'
SLAVE_COUNT="$(cat /sys/devices/w1_bus_master1/w1_master_slave_count)"
SLAVE_PATH="/sys/devices/w1_bus_master1/$(cat /sys/devices/w1_bus_master1/w1_master_slaves|grep '[0-9]*')"

echo "w1_master_slave_count: $SLAVE_COUNT"
echo "w1_master_slaves: $(cat /sys/devices/w1_bus_master1/w1_master_slaves)"
echo

if [ "$SLAVE_COUNT" = 0 ]; then
        STATUS=1
        echo -e "$script: FAIL One wire slave count is 0\n"
	print_status
	exit $STATUS
elif cat /sys/devices/w1_bus_master1/w1_master_slaves|grep -sq 'not found'; then
        STATUS=1
        echo -e "$script: FAIL w1_master_slaves says \"not found\"\n"
	print_status
	exit $STATUS
elif [ ! -e $SLAVE_PATH ]; then
        STATUS=1
        echo -e "$script: FAIL One wire Slave Not Found at $SLAVE_PATH\n"
	print_status
	exit $STATUS
fi

echo -e "$script: PASS One Wire Slave Found $SLAVE_PATH\n"

if [ ! -e $SLAVE_PATH/eeprom ]; then
        STATUS=1
        echo "$script: FAIL One wire Slaves eeprom file Not Found"
	echo "ls $SLAVE_PATH"
	ls $SLAVE_PATH
elif [ ! -w $SLAVE_PATH/eeprom ]; then
        STATUS=1
        echo "$script: FAIL One wire Slaves eeprom file Not writable"
	echo "ls -lh $SLAVE_PATH/eeprom"
	ls -lh $SLAVE_PATH/eeprom
else
	echo "Wrting \"$MESSAGE\" to eeprom"
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

echo
print_status
exit $STATUS
