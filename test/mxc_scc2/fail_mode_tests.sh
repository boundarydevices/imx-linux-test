#!/bin/bash
# Initialize the test accounting environment
. test_subs.sh

. scc2_setup.sh

setup_offsets

# Dump all registers
pos_test scc2_test.out -Lr


# Verify that a register can be written and does change
start_test Changing SCM_CIPHER_BLACK_START register to 4
scc2_test.out -W${SCM_CIPHER_BLACK_START}:4
length_value=`scc2_test.out -S+Q -R${SCM_CIPHER_BLACK_START}`
if [  -z $length_value ]; then
    record_failure Reading result of changed SCM_CIPHER_BLACK_START failed.
else
    if [ $length_value -eq 4 ]; then
	record_success Verified result of changed SCM_CIPHER_BLACK_START.
    else
	record_failure Read of SCM_CIPHER_BLACK_START return $length_value.
    fi
fi

# Verify it can changed to something else
start_test Changing SCM_CIPHER_BLACK_START register to 8
scc2_test.out -W${SCM_CIPHER_BLACK_START}:8
length_value=`scc2_test.out -S+Q -R${SCM_CIPHER_BLACK_START}`
if [  -z $length_value ]; then
    record_failure Reading result of changed SCM_CIPHER_BLACK_START failed.
else
    if [ $length_value -eq 8 ]; then
	record_success Verified result of changed SCM_CIPHER_BLACK_START.
    else
	record_failure Read of SCM_CIPHER_BLACK_START return $length_value.
    fi
fi


# This will invoke the Software Alarm and put the SMN into FAIL mode
pos_test scc2_test.out -La


# Read all 'safe' registers when SCC has gone to alarm/FAILED state
pos_test scc2_test.out -Ls


# Check that the state has transitioned to FAIL as expected
start_test Testing that the SCC state has transitioned to FAIL
status_value=`scc2_test.out -S+Q -R${SMN_STATUS}`
if [  -z $status_value ]; then
    record_failure Reading SMN_STATUS register failed
else
    if [ $((0x$status_value & 9)) -eq 9 ]; then
	record_success Part transitioned to FAIL state properly
    else
	record_failure Read of SMN_STATUS return $status_value.
    fi
fi

# Let the world know how things went
print_test_results
