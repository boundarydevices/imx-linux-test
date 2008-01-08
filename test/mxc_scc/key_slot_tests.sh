#!/bin/sh

#set -x

################################################################
#
#
#  Copyright 2005, Freescale Semiconductor, Inc.
#
################################################################


# Initialize the test accounting environment
. test_subs.sh

OWNER_ID=01234567fedcba98
KEY_VALUE8=ffeeddcc00112233
KEY_VALUE16=ffaaeebbdd44cc550066117722883399ffaaeebbdd44cc5500661177228833992
SHORT_KEY=1234

############################################################################
#
# Tests
#
############################################################################
# Test strategy - Encrypt / Decrypt feature of SCC driver

echo Allocate slot
pos_test scc_test.out -Ka8:$OWNER_ID > /tmp/slot_$$
cat /tmp/slot_$$

SLOT=`grep "^Slot.*is now al" /tmp/slot_$$`
SLOT=`echo $SLOT | sed -e 's/^Slot //' -e 's/ at .* is now allocated$//'`
if [ -z $SLOT ]; then SLOT=-1; fi

# Load slot with a key
pos_test scc_test.out -Kl$SLOT:$OWNER_ID:r:$KEY_VALUE8

# Make invalid attempt to deallocate it (bad uid)
neg_test scc_test.out -K$SLOT:1234

# Try to load a too-large key into the slot
neg_test scc_test.out -Kl$SLOT:$OWNER_ID:r:$KEY_VALUE16

# "Unload" a black version of our key
pos_test scc_test.out -Ku$SLOT:$OWNER_ID > /tmp/key1_$$
cat /tmp/key1_$$
BLACK1=`grep "^Encrypted val" /tmp/key1_$$ | sed -e 's/Encrypted value is //'`
if [ -z $BLACK1 ]; then BLACK1=a; fi

# Load a different key into the slot
pos_test scc_test.out -Kl$SLOT:$OWNER_ID:r:$SHORT_KEY

# Unload this value
pos_test scc_test.out -Ku$SLOT:$OWNER_ID > /tmp/key2_$$
cat /tmp/key2_$$
BLACK2=`grep "^Encrypted val" /tmp/key2_$$ | sed -e 's/Encrypted value is //'`
if [ -z $BLACK2 ]; then BLACK2=b; fi

# Make sure this key has different black version
neg_test cmp /tmp/key1_$$ /tmp/key2_$$

# Load original key in by its black version
pos_test scc_test.out -Kl$SLOT:$OWNER_ID:b:$BLACK1

# Unload it
pos_test scc_test.out -Ku$SLOT:$OWNER_ID > /tmp/key3_$$
BLACK3=`grep "^Encrypted val" /tmp/key3_$$ | sed -e 's/Encrypted value is //'`
if [ -z $BLACK3 ]; then echo Broken > /tmp/key3_$$; fi

# Compare with original version
pos_test cmp /tmp/key1_$$ /tmp/key3_$$

# Deallocate slot
pos_test scc_test.out -Kd$SLOT:$OWNER_ID

# Clean up
rm -f /tmp/key?_$$ /tmp/slot_$$

echo
echo "************************************************************************"
echo
echo "Tests complete"
echo

print_test_results

return
