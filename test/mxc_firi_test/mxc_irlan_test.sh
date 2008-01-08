#!/bin/sh

kernel_ver=2.6.22

if [ -f /lib/modules/$kernel_ver/kernel/drivers/net/irda/mxc_ir.ko ]; then
	echo "FIR detected"
	modprobe irda
	insmod /lib/modules/$kernel_ver/kernel/net/irda/irlan/irlan.ko access=2
	if [ $1 != 2 ]; then
	  ifconfig irlan0 10.0.0.1 netmask 255.255.255.0 broadcast 10.0.0.255
	else
	  ifconfig irlan0 10.0.0.2 netmask 255.255.255.0 broadcast 10.0.0.255
	fi
	insmod /lib/modules/$kernel_ver/kernel/drivers/net/irda/mxc_ir.ko
	ifconfig irda0 up
	echo 1 > /proc/sys/net/irda/discovery
else
	echo "Starting SIR"
	if grep 'MXC275-30 EVB' /proc/cpuinfo; then
		export IRDA_PORT=/dev/ttymxc0
	elif grep 'MXC91131 EVB' /proc/cpuinfo; then
		export IRDA_PORT=/dev/ttymxc1
	elif grep 'MX31' /proc/cpuinfo; then
		export IRDA_PORT=/dev/ttymxc1
	elif grep 'MX27' /proc/cpuinfo; then
		export IRDA_PORT=/dev/ttymxc2
	elif grep 'MXC300-20 EVB' /proc/cpuinfo; then
		export IRDA_PORT=/dev/ttymxc2
	else
		export IRDA_PORT=/dev/ttymxc3
	fi
	echo $IRDA_PORT

	stty -F $IRDA_PORT -echo
	modprobe irtty-sir
	insmod /lib/modules/$kernel_ver/kernel/net/irda/irlan/irlan.ko access=2
	if [ $1 != 2 ]; then
	  ifconfig irlan0 10.0.0.1 netmask 255.255.255.0 broadcast 10.0.0.255
	else
	  ifconfig irlan0 10.0.0.2 netmask 255.255.255.0 broadcast 10.0.0.255
	fi
	irattach $IRDA_PORT -s
fi
