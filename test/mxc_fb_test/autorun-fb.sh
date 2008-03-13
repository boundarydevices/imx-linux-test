#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

# devnode test
check_devnode "/dev/fb0"

export TERM=linux

# Turn off blanking
setterm -blank 0 > /dev/tty0


# Blank test
echo FB Blank test
echo -n 3 > /sys/class/graphics/fb0/blank
echo Screen should be off
sleep 1
echo -n 0 > /sys/class/graphics/fb0/blank

# Color tests
echo FB Color test
if [ "$(platform)" = MXC27530EVB ]; then
	bpp_list="16 24"
else
	bpp_list="16 24 32"
fi
for bpp in $bpp_list;
do
	echo Setting FB to $bpp-bpp
	echo -n $bpp > /sys/class/graphics/fb0/bits_per_pixel
	if ! grep -sq $bpp /sys/class/graphics/fb0/bits_per_pixel;
	then
		echo FAIL - Unable to set bpp
		STATUS=1
	fi
	setterm -inversescreen on > /dev/tty0
	setterm -foreground red > /dev/tty0
	setterm -clear all > /dev/tty0
	echo Screen is Red > /dev/tty0
	sleep 1
	setterm -foreground blue > /dev/tty0
	setterm -clear all > /dev/tty0
	echo Screen is Blue > /dev/tty0
	sleep 1
	setterm -foreground green > /dev/tty0
	setterm -clear all > /dev/tty0
	echo Screen is Green > /dev/tty0
	sleep 1
done

# Pan test
#
echo FB panning test
echo 240,640 > /sys/class/graphics/fb0/virtual_size
if ! grep -sq 240,640 /sys/class/graphics/fb0/virtual_size;
then
	# MXC27530 Uses IRAM and can't support panning
	if [ "$(platform)" = MXC27530EVB ]; then
		print_status
		exit $STATUS
	fi
	echo FAIL - Unable to set virtual size
	STATUS=1
fi

for i in $(seq 1 50); do
	echo This is line $i. > /dev/tty0
done
for i in $(seq 1 320); do
	echo 0,$i > /sys/class/graphics/fb0/pan
	if ! grep -sq $i /sys/class/graphics/fb0/pan;
	then
		echo FAIL - Unable to pan
		STATUS=1
	fi
done

print_status
exit $STATUS
