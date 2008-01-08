#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

# Inserting the RNG Module.
insmod_test /lib/modules/$(kernel_version)/test/rng_test_driver.ko

# protocol test cases
# Reading Safe Registers.
run_testcase "./rng_test -S"

# Reading All RNG Registers.
echo "Reading Version ID Register"
run_testcase "./rng_test -R0"

echo "Reading Command Register"
run_testcase "./rng_test -R4"

echo "Reading Control Register"
run_testcase "./rng_test -R8"

echo "Reading Status Register"
run_testcase "./rng_test -Rc"

echo "Reading Error Status Register"
run_testcase "./rng_test -R10"

echo "Reading FIFO data Register"
run_testcase "./rng_test -R14"

echo "Reading Verification Control Register"
run_testcase "./rng_test -R20"

echo "Reading Oscillator Counter Control Register"
run_testcase "./rng_test -R28"

echo "Reading Oscillator Counter Register"
run_testcase "./rng_test -R2c"

echo "Reading Oscillator Counter Status Register"
run_testcase "./rng_test -R30"

# Generating Random Number in User Mode
for CASE in 4 8 50 100 200 500 ; do
	echo "Reading $CASE Random Numbers"
	run_testcase "./rng_test -E$CASE"
done

# Generating Random Number in Kernel Mode
for CASE in 4 8 50 100 200 500 ; do
	echo "Reading $CASE Random Numbers as kernel mode"
	run_testcase "./rng_test -Ok -E$CASE"
done

# Adding Entropy in User Mode
for CASE in 4 8 C AB 2CC 5AB ; do
	echo "Adding Entropy in User Mode"
	run_testcase "./rng_test -Z$CASE"
done

# Adding Entropy in Kernel Mode
for CASE in 4 8 C AB 2CC 5AB ; do
	echo "Adding Entropy in Kernel Mode"
	run_testcase "./rng_test -Ok -Z$CASE"
done

# Writing in to RNG Registers
echo "Writing new value to Version Id Register"
run_testcase "./rng_test -W0:4"

echo "Writing New value to Command Register"
run_testcase "./rng_test -W4:3"

print_status
exit $STATUS
