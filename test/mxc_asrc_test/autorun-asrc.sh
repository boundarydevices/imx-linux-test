#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

check_devnode "/dev/mxc_asrc"
check_devnode "/dev/snd/pcmC0D0p"

if [ "$STATUS" = 0 ]; then
run_testcase " ./mxc_asrc_test.out -to 48000 audio8k16S.wav /tmp/s48.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " aplay -N -M /tmp/s48.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " ./mxc_asrc_test.out -to 96000 /tmp/s48.wav /tmp/s96.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " aplay -N -M /tmp/s96.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " rm /tmp/s48.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " ./mxc_asrc_test.out -to 88200 /tmp/s96.wav /tmp/s882.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " aplay -N -M /tmp/s882.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " rm /tmp/s96.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " ./mxc_asrc_test.out -to 44100 /tmp/s882.wav /tmp/s441.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " aplay -N -M /tmp/s441.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " rm /tmp/s882.wav /tmp/s441.wav /tmp/raw.txt"
fi
print_status
exit $STATUS
