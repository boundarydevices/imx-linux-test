#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

check_devnode "/dev/mxc_asrc"
check_devnode "/dev/snd/pcmC0D0p"

if [ "$STATUS" = 0 ]; then
run_testcase " ./mxc_asrc_test.out -to 48000 audio8k16S.wav /dev/s48.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " aplay -N -M /dev/s48.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " ./mxc_asrc_test.out -to 96000 /dev/s48.wav /dev/s96.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " aplay -N -M /dev/s96.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " rm /dev/s48.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " ./mxc_asrc_test.out -to 88200 /dev/s96.wav /dev/s882.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " aplay -N -M /dev/s882.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " rm /dev/s96.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " ./mxc_asrc_test.out -to 44100 /dev/s882.wav /dev/s441.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " aplay -N -M /dev/s441.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " rm /dev/s882.wav /dev/s441.wav /dev/raw.txt"
fi
print_status
exit $STATUS
