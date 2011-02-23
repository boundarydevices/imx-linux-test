#!/bin/bash
## This command should be checked for error status.  It has numerous side
## effects relating to exportation of symbols
##
## The command 'scc2_test' must be in its PATH


############################################################################
#
# Internal functions
#
############################################################################

# Initialize variable names for Register offsets
init_offsets()
{
    SCM_VERSION=0000
    SCM_INTERRUPT_CTRL=0008
    SCM_COMMAND_STATUS=000C
    SCM_ERROR_STATUS=0010
    SCM_DMA_FAULT_ADDR=0014
    SCM_PARTITION_OWNERS=0018
    SCM_PARTITIONS_ENGAGED=001C
    SCM_UNIQUE_NO_0=0020
    SCM_UNIQUE_NO_1=0024
    SCM_UNIQUE_NO_2=0028
    SCM_UNIQUE_NO_3=002C
    SCM_ZEROIZE_CMD=0050
    SCM_CIPHER_CMD=0055
    SCM_CIPHER_BLACK_START=0058
    SCM_INTERNAL_DEBUG=005C
    SCM_CIPHER_IV_0=0060
    SCM_CIPHER_IV_1=0064
    SCM_CIPHER_IV_2=0068
    SCM_CIPHER_IV_3=006C
    SCM_SMID_BASE=0080
    SCM_MAP_BASE=0084

    SMN_STATUS=0100
    SMN_COMMAND=0104
    SMN_SEQUENCE_START=0108
    SMN_SEQUENCE_END=010C
    SMN_SEQUENCE_CHECK=0110
    SMN_BIT_COUNT=0114
    SMN_BB_INCREMENT=0118
    SMN_BB_DECREMENT=011C
    SMN_COMPARE_SIZE=0120
    SMN_PLAINTEXT_CHECK=0124
    SMN_CIPHERTEXT_CHECK=0128
    SMN_TIMER_IV=012C
    SMN_TIMER_CTRL=0130
    SMN_SECURITY_VIOLATION=0134
    SMN_SECURITY_TIMER=0138
    SMN_HAC=013C
}


## Export the SCC register symbols to the shell
do_exports()
{
    export SCM_VERSION
    export SCM_INTERRUPT_CTRL
    export SCM_COMMAND_STATUS
    export SCM_ERROR_STATUS
    export SCM_DMA_FAULT_ADDR
    export SCM_PARTITION_OWNERS
    export SCM_PARTITIONS_ENGAGED
    export SCM_UNIQUE_NO_0
    export SCM_UNIQUE_NO_1
    export SCM_UNIQUE_NO_2
    export SCM_UNIQUE_NO_3
    export SCM_ZEROIZE_CMD
    export SCM_CIPHER_CMD
    export SCM_CIPHER_BLACK_START
    export SCM_INTERNAL_DEBUG
    export SCM_CIPHER_IV_0
    export SCM_CIPHER_IV_1
    export SCM_CIPHER_IV_2
    export SCM_CIPHER_IV_3
    export SCM_SMID_BASE
    export SCM_MAP_BASE

    export SMN_STATUS
    export SMN_COMMAND
    export SMN_SEQUENCE_START
    export SMN_SEQUENCE_END
    export SMN_SEQUENCE_CHECK
    export SMN_BIT_COUNT
    export SMN_BB_INCREMENT
    export SMN_BB_DECREMENT
    export SMN_COMPARE_SIZE
    export SMN_PLAINTEXT_CHECK
    export SMN_CIPHERTEXT_CHECK
    export SMN_TIMER_IV
    export SMN_TIMER_CTRL
    export SMN_SECURITY_VIOLATION
    export SMN_SECURITY_TIMER
    export SMN_HAC

    return 0
}

setup_offsets()
{
    init_offsets
    do_exports

    return 0
}
