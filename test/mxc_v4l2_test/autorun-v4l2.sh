#!/bin/sh

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

if [ "$(platform)" = MXC30031ADS ]; then
	TEST_CAMERA=0;
else
	TEST_CAMERA=1;
fi

# devnode test
if [ $TEST_CAMERA = 1 ]; then
check_devnode "/dev/video0"
fi
check_devnode "/dev/video16"

# Turn off fb blanking
echo -e "\033[9;0]" > /dev/vc/0

if [ "$(platform)" = IMX27ADS ]; then
DISPLAY=0;
else
DISPLAY=3;
fi
#
# V4L2 Output Tests
#
# SDC output size test cases
for SIZE in 32 64 80 96 112 128 144 160 176 192 208 224 240; do
	run_testcase "./mxc_v4l2_output.out -iw 128 -ih 128 -ow $SIZE -oh $SIZE -d $DISPLAY -r 0"
done

# SDC input size test cases
for SIZE in 32 40 48 64 80 96 112 128 144 160 176 192 208 224 240; do
	run_testcase "./mxc_v4l2_output.out -iw $SIZE -ih $SIZE -ow 120 -oh 120 -d $DISPLAY -r 0"
done

# SDC output rotation test cases
for ROT in 0 1 2 3 4 5 6 7; do
	run_testcase "./mxc_v4l2_output.out -iw 352 -ih 288 -ow 240 -oh 320 -d $DISPLAY -r $ROT"
done

# SDC max input size test case
if [ "$(platform)" = IMX27ADS ]; then
	run_testcase "./mxc_v4l2_output.out -iw 640 -ih 512 -ow 240 -oh 320 -d $DISPLAY -fr 60 -r 4"
else
	run_testcase "./mxc_v4l2_output.out -iw 480 -ih 640 -ow 240 -oh 320 -d 4 -fr 60"
	run_testcase "./mxc_v4l2_output.out -iw 720 -ih 512 -ow 240 -oh 184 -d $DISPLAY -fr 60"
fi

if [ $TEST_CAMERA = 1 ]; then

# V4L2 Capture Tests
run_testcase "./mxc_v4l2_overlay.out -iw 640 -ih 480 -ow 240 -oh 320 -r 4 -fr 30 -fg -t 10"
run_testcase "./mxc_v4l2_overlay.out -iw 640 -ih 480 -ow 240 -oh 320 -r 4 -fr 30 -t 10"

for ROT in 0 1 2 3 4 5 6 7; do
	run_testcase "./mxc_v4l2_overlay.out -iw 640 -ih 480 -ow 240 -oh 184 -r $ROT -fr 30 -fg -t 5"
done

for POS in 0 4 8 16 32 64 128; do
	run_testcase "./mxc_v4l2_overlay.out -iw 640 -ih 480 -ot $POS -ol $POS -ow 80 -oh 60 -fr 30 -fg -t 5"
done

fi


print_status
exit $STATUS
