#!/bin/sh


################################################################
#
#
#  Copyright 2005, Freescale Semiconductor, Inc.
#
################################################################

# Initialize the test accounting environment
. test_subs.sh


. scc_setup.sh

# set up
determine_offsets
if [ $? -ne 0 ]; then
    echo Test failed to initialize
    exit $?
fi


# Dump all registers
pos_test scc_test.out -Lr


# Verify that a register can be written and does change
start_test Changing SCM_LENGTH register to 4
scc_test.out -W${SCM_LENGTH}:4
length_value=`scc_test.out -S+Q -R${SCM_LENGTH}`
if [  -z $length_value ]; then
    record_failure Reading result of changed SCM_LENGTH failed.
else
    if [ $length_value -eq 4 ]; then
	record_success Verified result of changed SCM_LENGTH.
    else
	record_failure Read of SCM_LENGTH return $length_value.
    fi
fi

# Verify it can changed to something else
start_test Changing SCM_LENGTH register to 8
scc_test.out -W${SCM_LENGTH}:8
length_value=`scc_test.out -S+Q -R${SCM_LENGTH}`
if [  -z $length_value ]; then
    record_failure Reading result of changed SCM_LENGTH failed.
else
    if [ $length_value -eq 8 ]; then
	record_success Verified result of changed SCM_LENGTH.
    else
	record_failure Read of SCM_LENGTH return $length_value.
    fi
fi


# This will invoke the Software Alarm and put the SMN into FAIL mode
pos_test scc_test.out -La


# Read all 'safe' registers when SCC has gone to alarm/FAILED state
pos_test scc_test.out -Ls


echo
echo
echo "The rest of tests should have errors (but pass)"

# Test SCM Length and SMN Sequence Start
neg_test scc_test.out -R${SCM_LENGTH}
neg_test scc_test.out -R${SMN_SEQUENCE_START}

# Let the world know how things went
print_test_results
