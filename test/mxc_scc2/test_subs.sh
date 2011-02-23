#!/bin/bash

################################################################
## @file test_subs.sh
## This script provides some accounting methods for running
## tests, checking their results, and tallying the results.
#
# To be of any value whatsoever, this script must be run in the
# same process as the calling script, i.e. invoked by
# . test_subs.sh
#
################################################################


# Initialize counters
test_count=0
test_pass_count=0


################################################################
# fn start_test()
################################################################
##
## Initialize a test and account for its launch.
##
## Arguments: Whatever 'test name' should be displayed
##
start_test()
{

    echo
    echo "****************************************"
    echo
    echo Running test: $*

    # Evaluate this arithmetically!
    # (This seems to be the most 'portable' method)
    test_count=$(($test_count+1))

}


################################################################
# fn run_test()
################################################################
##
## Run a test and account for its launch.
##
## Arguments: Whatever command line should be executed as the test.
##
## Note that quoted commands will NOT work well, as quotes
## will be stripped.
##
run_test()
{
    start_test $*

    # Actual test launch
    $*

    # return value is that of the test command
    return
}


################################################################
# fn pos_test()
################################################################
## Run a positive test -- verify no error
##
## Arguments: Whatever command line should be executed as the test.
##
## Note that quoted commands will NOT work well, as quotes
## will be stripped.
##
pos_test()
{

    run_test $*
    if [ $? -eq 0 ]; then
        test_pass_count=$(($test_pass_count+1))
        echo "Test Passed"
    else
        echo "Test Failed"
    fi
}


################################################################
#fn neg_test()
################################################################
## Run a negative test -- look for error
##
## Arguments: Whatever command line should be executed as the test.
##
## Note that quoted commands will NOT work well, as quotes
## will be stripped.
##
neg_test()
{
    run_test $*
    if [ $? -ne 0 ]; then
        test_pass_count=$(($test_pass_count+1))
        echo "Test Passed"
    else
        echo "Test Failed"
    fi
}


################################################################
# fn record_failure()
################################################################
##
## Record a failed result of a test begun by #start_test
##
## Arguments: Message to print
##
record_failure()
{
    echo $*
    echo "Test Failed"
}


################################################################
# fn record_success()
################################################################
##
## Record a successful result of a test begun by #start_test
##
## Arguments: Message to print
##
record_success()
{
    echo $*
    echo "Test Passed"
    test_pass_count=$(($test_pass_count+1))
}


################################################################
# fn print_test_results()
################################################################
##
## Print summary of all tests launched
##
print_test_results()
{
    echo ${test_pass_count} " tests passed out of " ${test_count} " launched"

    if [ ${test_count} -eq ${test_pass_count} ]; then
        return 0                # no errors
    else
        return 1                # At least one error
    fi
}

