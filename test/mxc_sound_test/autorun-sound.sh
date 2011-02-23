
#!/bin/bash

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

check_devnode "/dev/dsp"
check_devnode "/dev/adsp"
check_devnode "/dev/audio"
check_devnode "/dev/mixer"


if [ "$STATUS" = 0 ]; then
run_testcase " arecord -N -M -r 8000 -f S16_LE -c 1 -d 8 test.wav"
fi


if [ "$STATUS" = 0 ]; then
run_testcase " aplay -N -M test.wav"
fi

if [ "$STATUS" = 0 ]; then
run_testcase " aplay -N -M -D hw:0,1 test.wav"
fi

print_status
exit $STATUS
