#!/bin/sh

#
# If one test case fails, this script keeps running the rest of the
# tests.  Exit status is PASS only if all cases pass.
#
run_testcase()
{
	script=$(basename $0)
	echo "Running test case: $1"
	if ! $1 ; then
		STATUS=1
		printf "$script: FAIL test case: $1\n\n"
	else
		printf "$script: PASS test case: $1\n\n"
	fi

	# sleep a little to let autotest.pl script catch up with the logging.
	sleep 2
}

#
# Use this for running test cases that are expected to fail.
#
expect_fail()
{
	script=$(basename $0)
	echo "Running test case - expect it to fail: $1"
	if ! $1 ; then
		printf "$script: PASS test case returns fail as expected: $1\n\n"
	else
		STATUS=1
		printf "$script: FAIL test case returns pass when it should have failed: $1\n\n"
	fi

	# sleep a little to let autotest.pl script catch up with the logging.
	sleep 2
}

check_devnode()
{
	script=$(basename $0)
	echo "Checking for devnode: $1"
	if [ ! -e $1 ]; then
		STATUS=1
		printf "$script: FAIL devnode not found: $1\n\n"
	else
		printf "$script: PASS devnode found: $1\n\n"
	fi

	# sleep a little to let autotest.pl script catch up with the logging.
	sleep 2
}

check_executable()
{
	script=$(basename $0)
	echo "Checking for executable: $1"
	if [ ! -x $1 ]; then
		STATUS=1
		if [ -e $1 ]; then
			printf "$script: FAIL executable found but not executable: $1\n\n"
			ls -lh $1
		else
			printf "$script: FAIL executable not found: $1\n\n"
		fi
	else
		printf "$script: PASS executable found: $1\n\n"
	fi

	# sleep a little to let autotest.pl script catch up with the logging.
	sleep 2
}

#
# You'll need to specify full path to the module
#
# For one parameter, quotes aren't needed, i.e.:
#    insmod_test /unit_tests/modules/scc_test_driver.ko
#
# If the insmod has parameters call this with module and parameters in quotes like this:
#  insmod_test "/lib/modules/2.6.18.1/kernel/net/irda/irlan/irlan.ko access=2"
#
insmod_test()
{
	module_test insmod "$1"
}

# modprobe_test does not need the full module path.
modprobe_test()
{
	module_test modprobe "$1"
}

# This is used by insmod_test and modprobe_test, don't call it directly.
module_test()
{
	script=$(basename $0)
	mod_util=$1
	module="$2"

	if [ "$mod_util" != insmod ] && [ "$mod_util" != modprobe ]; then
		echo "scripting error in module_test: $0: module_test $1 $2"
		exit
	fi

        echo "Attempting $mod_util $module"
        if ! $mod_util $module 2>&1; then
                STATUS=1
                echo "$script: FAIL: $mod_util returned error"
        else
                echo "$script: PASS: $mod_util returned success"
        fi

	# Get the module name, without any insmod parameters, and without the path.
	mod_name=$(echo "$module"|cut -d\  -f1|xargs basename)

	# Get rid of the suffix (.ko or whatever)
	mod_name=${mod_name%.*}

        # We check lsmod for the module even if the above fails
        # the lsmod in the log file may help debugging.
        echo "Checking lsmod for the module $mod_name"
        if lsmod|grep "$mod_name"; then
                echo "PASS: module appears in lsmod"
        else
                echo "FAIL: module does not appear in lsmod"
                lsmod
                STATUS=1
        fi
	echo

	# sleep a little to let autotest.pl script catch up with the logging.
	sleep 2
}

print_status()
{
	script=$(basename $0)
	if [ "$STATUS" = 0 ]; then
	        printf "$script: Exiting PASS\n\n"
	else
	        printf "$script: Exiting FAIL\n\n"
	fi
}

#
# This echos the plaftorm name in all caps:
# MXC27530EVB, MXC27520EVB, IMX27ADS, IMX31ADS, MXC91131EVB, MXC30030EVB, MXC30020EVB
#
# for example use in an if statement like:  if [ "$(platform)" = MXC27530EVB ]; then
#
# This is only for use where a test needs to run differently for different platforms.
# To keep a test from running at all for a platform, add it to the EXCLUDES list in
# the Makefile.
#
platform()
{
	# Get Hardware string from proc, knock out spaces, periods, hyphens, capitalize it all.
	plat=$(egrep '^Hardware' /proc/cpuinfo|sed -r 's,.*Freescale ,,'| \
		sed 's,i,I,'|sed -r 's,( |\.|\-),,g')

	revision=$(egrep '^Revision' /proc/cpuinfo|cut -d' ' -f2)

	case $plat in
		MX31* ) case $revision in
				31* )	plat=IMX31ADS ;;
				* )	plat=IMX32ADS ;;
			esac ;;

		MX253StackBoard )	plat=IMX25_3STACK ;;
		MX313StackBoard )	plat=IMX31_3STACK ;;
		MX353StackBoard )	plat=IMX35_3STACK ;;
		MX373StackBoard )	plat=IMX37_3STACK ;;
		MX513StackBoard )       plat=IMX51_3STACK ;;

		* )     ;;
	esac
	echo "$plat"
}

# Check that this test exists in this rootfs for this platform.
# If so, find the test app and echo the testapp and path
look_up_test_by_id()
{
	if echo "$id"|grep -sq '#'; then
		return
	fi
	if ! echo "$id"|grep -sq 'FSL-UT-'; then
		return
	fi

	test="$(grep "$1" /unit_tests/autorun-suite.txt|cut -d: -f2-)"

	testapp=$(basename "$test"|cut -d\  -f1)

	params="$(echo "$test"|cut -s -d\  -f2-)"

	app_with_path="$(find . -name $testapp|head -1)"

	if [ -z $app_with_path ]; then
		return
	fi

	app_with_path="$PWD/$(echo $app_with_path|sed 's,./,,')"

	echo "$app_with_path $params"
}

# returns kernel version, such as 2.6.22.
kernel_version()
{
	cat /proc/version|cut -d' ' -f3
}
