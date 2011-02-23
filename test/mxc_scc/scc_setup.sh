#!/bin/bash

## This command should be checked for error status.  It has numerous side
## effects relating to exportation of symbols
##
## The command 'scc_test' must be in its PATH


############################################################################
#
# Internal functions
#
############################################################################

# Initialize variable names for Register offsets
init_offsets()
{
    SCM_RED_START=0000
    SCM_BLACK_START=0004
    SCM_LENGTH=0008
    SCM_CONTROL=000C
    SCM_STATUS=0010
    SCM_ERROR_STATUS=0014
    SCM_INTERRUPT_CTRL=0018
    SCM_CONFIGURATION=001C
    SCM_INIT_VECTOR_0=0020
    SCM_INIT_VECTOR_1=0024
    SCM_RED_MEMORY=0400
    SCM_BLACK_MEMORY=0800

    SMN_STATUS=0000
    SMN_COMMAND=0004
    SMN_SEQUENCE_START=0008
    SMN_SEQUENCE_END=000C
    SMN_SEQUENCE_CHECK=0010
    SMN_BIT_COUNT=0014
    SMN_BITBANK_INC_SIZE=0018
    SMN_BITBANK_DECREMENT=001C
    SMN_COMPARE_SIZE=0020
    SMN_PLAINTEXT_CHECK=0024
    SMN_CIPHERTEXT_CHECK=0028
    SMN_TIMER_IV=002C
    SMN_TIMER_CONTROL=0030
    SMN_DEBUG_DETECT_STAT=0034
    SMN_TIMER=0038
}


## Export the SCC register symbols to the shell
do_exports()
{
    export SCM_RED_START
    export SCM_BLACK_START
    export SCM_LENGTH
    export SCM_CONTROL
    export SCM_STATUS
    export SCM_ERROR_STATUS
    export SCM_INTERRUPT_CTRL
    export SCM_CONFIGURATION
    export SCM_INIT_VECTOR_0
    export SCM_INIT_VECTOR_1
    export SCM_RED_MEMORY
    export SCM_BLACK_MEMORY

    export SMN_STATUS
    export SMN_COMMAND
    export SMN_SEQUENCE_START
    export SMN_SEQUENCE_END
    export SMN_SEQUENCE_CHECK
    export SMN_BIT_COUNT
    export SMN_BITBANK_INC_SIZE
    export SMN_BITBANK_DECREMENT
    export SMN_COMPARE_SIZE
    export SMN_PLAINTEXT_CHECK
    export SMN_CIPHERTEXT_CHECK
    export SMN_TIMER_IV
    export SMN_TIMER_CONTROL
    export SMN_DEBUG_DETECT_STAT
    export SMN_TIMER

    return 0
}


## Change the offsets for the SMN and SCM registers, based upon platform
##
##
determine_offsets()
{
    return_value=1              # default to error
    platform=0                  # unknown platform

    init_offsets

    # Try to read the SCM Configuration register.  The SCC driver will cause
    # this to generate an error on one platform, succeed on another, since the
    # SCM and SMN base addresses are swapped on some platforms.
    config=`scc_test -S+Q -R1c`

    # See whether command executed successful
    if [ $? -eq 0 ]; then
        # No errors.  Good.  Check its value.
        # (a008008 value is for the broken VIRTIO mxc platform)
        if [ $config = 09004008 -o $config = 0a008008 ]; then
            platform=1          # SCM First
        fi
    else
        # Try again with same register, but in the other bank
        config=`scc_test -S+Q -R101c`
        if [ $? -eq 0 ]; then
        # No errors.  Good.  Check its value.
            if [ $config = 09004008 ]; then
                platform=2              # SMN First
            fi
        fi                      # (else) if return eq 0
    fi                          # if return eq 0

    # Use platform determination to update values
    if [ $platform -eq 1 ]; then

        # must bump SMN offsets
        SMN_STATUS=1000
        SMN_COMMAND=1004
        SMN_SEQUENCE_START=1008
        SMN_SEQUENCE_END=100C
        SMN_SEQUENCE_CHECK=1010
        SMN_BIT_COUNT=1014
        SMN_BITBANK_INC_SIZE=1018
        SMN_BITBANK_DECREMENT=101C
        SMN_COMPARE_SIZE=1020
        SMN_PLAINTEXT_CHECK=1024
        SMN_CIPHERTEXT_CHECK=1028
        SMN_TIMER_IV=102C
        SMN_TIMER_CONTROL=1030
        SMN_DEBUG_DETECT_STAT=1034
        SMN_TIMER=1038
	do_exports
        return_value=0

    elif [ $platform -eq 2 ]; then

        # must bump SCM offsets
        SCM_RED_START=1000
        SCM_BLACK_START=1004
        SCM_LENGTH=1008
        SCM_CONTROL=100C
        SCM_STATUS=1010
        SCM_ERROR_STATUS=1014
        SCM_INTERRUPT_CTRL=1018
        SCM_CONFIGURATION=101C
        SCM_INIT_VECTOR_0=1020
        SCM_INIT_VECTOR_1=1024
        SCM_RED_MEMORY=1400
        SCM_BLACK_MEMORY=1800
	do_exports
        return_value=0

    else

        echo "Error: Cannot read SCM configuration register"

    fi                          # if platform eq 1

    return $return_value
}


determine_offsets
