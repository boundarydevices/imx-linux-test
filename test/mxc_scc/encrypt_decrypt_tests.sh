#!/bin/bash

#set -x

################################################################
#
#
#  Copyright 2005, Freescale Semiconductor, Inc.
#
################################################################


# Initialize the test accounting environment
. test_subs.sh


############################################################################
#
# Tests
#
############################################################################


# Test strategy - Encrypt / Decrypt feature of SCC driver

# Test a straightforward 24-byte encryption in ECB then CBC modes.
pos_test scc_test -S+p0 -Le
pos_test scc_test -S+p0 -S+m -Le

# Test various byte offsets for plaintext and ciphertext strings.
pos_test scc_test -S+p0 -S+o1 -Le
pos_test scc_test -S+p0 -S+o2 -Le
pos_test scc_test -S+p0 -S+o3 -Le

# Test various boundary cases in normal mode.
pos_test scc_test -S+l8 -Le
pos_test scc_test -S+l1016 -Le
pos_test scc_test -S+l1024 -Le
pos_test scc_test -S+l1032 -Le

# Test various boundary cases in verify mode - memory boundaries of 1K
# SCC RAM size.
pos_test scc_test -S+v -S+l1014 -Le
pos_test scc_test -S+v -S+l1015 -Le
pos_test scc_test -S+v -S+l1016 -Le
pos_test scc_test -S+v -S+l1017 -Le
pos_test scc_test -S+v -S+l1022 -Le
pos_test scc_test -S+v -S+l1023 -Le
pos_test scc_test -S+v -S+l1024 -Le
pos_test scc_test -S+v -S+l1025 -Le

# Test various boundary cases in verify mode - block boundaries within
# the 8-byte 3DES block - provide exact amount of ciphertext padding.
pos_test scc_test -S+v -S+p7  -S+l1  -Le
pos_test scc_test -S+v -S+p6  -S+l2  -Le
pos_test scc_test -S+v -S+p5  -S+l3  -Le
pos_test scc_test -S+v -S+p4  -S+l4  -Le
pos_test scc_test -S+v -S+p3  -S+l5  -Le
pos_test scc_test -S+v -S+p10 -S+l6  -Le
pos_test scc_test -S+v -S+p9  -S+l7  -Le
pos_test scc_test -S+v -S+p8  -S+l8  -Le


# Test some expected failures - Bad arguments to encrypt/decrypt:

echo
echo "Expected failures -- bad arguments"
#  - Plaintext count not multiple of block size in normal mode
neg_test scc_test -S+l1 -Le
neg_test scc_test -S+l10 -Le
neg_test scc_test -S+l1027 -Le
neg_test scc_test -S+l1028 -Le
neg_test scc_test -S+l1029 -Le
neg_test scc_test -S+l22 -Le
neg_test scc_test -S+l23 -Le

echo
echo "Expected failures - Insufficient space"
# - One byte too few of ciphertext space in normal mode encryption.
neg_test scc_test -S+p-1 -Le

# - One byte too few of ciphertext space in verify mode encryption.
neg_test scc_test -S+v -S+p6 -S+l1  -Le
neg_test scc_test -S+v -S+p5 -S+l2  -Le
neg_test scc_test -S+v -S+p4 -S+l3  -Le
neg_test scc_test -S+v -S+p3 -S+l4  -Le
neg_test scc_test -S+v -S+p2 -S+l5  -Le
neg_test scc_test -S+v -S+p9 -S+l6  -Le
neg_test scc_test -S+v -S+p8 -S+l7  -Le
neg_test scc_test -S+v -S+p7 -S+l8  -Le

# - One byte too few of output plaintext space in normal mode decryption.
neg_test scc_test -S-p-1 -Le

# - One byte too few of output plaintext space in verify mode decryption.
neg_test scc_test -S+v -S-p-1 -Le


# Test some verification failures by generating error in ciphertext before
# trying to decrypt in verify mode.
echo
echo "Expected failures - Verification failure"
neg_test scc_test -S+v -S+c -S+l1 -Le
neg_test scc_test -S+v -S+c -S+l1016 -Le
neg_test scc_test -S+v -S+c -S+l1020 -Le
neg_test scc_test -S+v -S+c -S+l1024 -Le
neg_test scc_test -S+v -S+c -S+l1028 -Le


echo
echo "************************************************************************"
echo
echo "Tests complete"
echo

print_test_results

return
