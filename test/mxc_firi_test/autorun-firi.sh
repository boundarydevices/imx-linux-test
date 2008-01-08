#! /bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

run_firi_setup()
{
	modprobe_test irda
	insmod_test "/lib/modules/$(kernel_version)/kernel/net/irda/irlan/irlan.ko access=2"
	run_testcase "ifconfig irlan0 10.0.0.1 netmask 255.255.255.0 broadcast 10.0.0.255"
	insmod_test /lib/modules/$(kernel_version)/kernel/drivers/net/irda/mxc_ir.ko
	run_testcase "ifconfig irda0 up"
}

run_firi_setup

print_status
exit $STATUS
