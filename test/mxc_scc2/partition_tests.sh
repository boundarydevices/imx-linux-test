#!/bin/sh

#set -x

# Initialize the test accounting environment
. test_subs.sh

OWNER_ID=01234567fedcba98
KEY_VALUE8=ffeeddcc00112233
KEY_VALUE16=ffaaeebbdd44cc550066117722883399
SHORT_KEY=1234

############################################################################
#
# Tests
#
############################################################################
# Test strategy - Run partition encrypt/decrypt tests

echo Test partition
pos_test scc2_test.out -Lp

echo
echo "************************************************************************"
echo
echo "Tests complete"
echo

print_test_results

return
