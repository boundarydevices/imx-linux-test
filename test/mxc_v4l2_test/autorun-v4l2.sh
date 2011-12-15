#!/bin/bash

source /unit_tests/test-utils.sh

start_time=`date`
echo =============== test start from $start_time ==============================

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
if [ "$(platform)" = "IMX35_3STACK" ]; then
insmod ipu_prp_enc
insmod ipu_prp_vf_sdc
insmod ipu_prp_vf_sdc_bg
insmod ipu_still
insmod ov2640_camera
insmod mxc_v4l2_capture
fi
check_devnode "/dev/video0"
fi
if [ "$(platform)" != IMX25_3STACK ]; then
check_devnode "/dev/video16"
fi

if ([ "$(platform)" = IMX51 ] || [ "$(platform)" = IMX53 ] \
	|| [ "$(platform)" = IMX6 ]); then
check_devnode "/dev/video17"
fi

# Turn off fb blanking
echo -e "\033[9;0]" > /dev/tty0

if [ "$(platform)" = IMX27ADS -o "$(platform)" = IMX25_3STACK ]; then
DISPLAY=0;
else
DISPLAY=3;
fi

if ([ "$(platform)" = IMX51 ] || [ "$(platform)" = IMX53 ] \
	|| [ "$(platform)" = IMX6 ]); then
DISPLAY=/dev/video17
fi

#
# V4L2 Output Tests
#

if ([ "$(platform)" = IMX51 ] || [ "$(platform)" = IMX53 ] \
	|| [ "$(platform)" = IMX6 ]); then
DISPW=`cat /sys/class/graphics/fb0/mode | awk -F ':' '{print $2}' | awk -F 'x' '{print $1}'`
DISPH=`cat /sys/class/graphics/fb0/mode | awk -F ':' '{print $2}' | awk -F 'x' '{print $2}' | awk -F 'p' '{print $1}'`
DISPW_DIV_2=`expr $DISPW / 2`
DISPW_DIV_8=`expr $DISPW / 8`
DISPH_DIV_2=`expr $DISPH / 2`
DISPH_DIV_8=`expr $DISPH / 8`

if [ "$FULLTEST" = '1' ]; then
IWCASE="$DISPW_DIV_8 $DISPW_DIV_2 $DISPW"
IHCASE="$DISPH_DIV_8 $DISPH_DIV_2 $DISPH"
OWCASE="$DISPW_DIV_8 $DISPW_DIV_2 $DISPW"
OHCASE="$DISPH_DIV_8 $DISPH_DIV_2 $DISPH"
else
IWCASE="$DISPW"
IHCASE="$DISPH"
OWCASE="$DISPW"
OHCASE="$DISPH"
fi

echo =========== do test on display $DISPW $DISPH ====================
for IW in $IWCASE; do
for IH in $IHCASE; do
for OW in $OWCASE; do
for OH in $OHCASE; do
for ROT in 0 90; do
for VF in 0 1; do
for HF in 0 1; do
	ICW=`expr $IW / 4`
	ICH=`expr $IH / 4`
	OCW=`expr $OW / 4`
	OCH=`expr $OH / 4`
	# resizing, rotation, flip test
	run_testcase "./mxc_v4l2_output.out -iw $IW -ih $IH -ow $OW -oh $OH -d $DISPLAY -r $ROT -vf $VF -hf $HF -fr 60"
	# resizing, rotation, flip with crop test
	run_testcase "./mxc_v4l2_output.out -iw $IW -ih $IH -cr $ICW $ICH $ICW $ICH 0 -ow $OW -oh $OH -ol $OCW -ot $OCH -d $DISPLAY -r $ROT -vf $VF -hf $HF -fr 60"
	# resizing, rotation, flip test for deinterlacing
	run_testcase "./mxc_v4l2_output.out -iw $IW -ih $IH -ow $OW -oh $OH -d $DISPLAY -r $ROT -vf $VF -hf $HF -v 0 -fr 60"
	run_testcase "./mxc_v4l2_output.out -iw $IW -ih $IH -ow $OW -oh $OH -d $DISPLAY -r $ROT -vf $VF -hf $HF -v 2 -fr 60"
	# resizing, rotation, flip with crop test for deinterlacing
	run_testcase "./mxc_v4l2_output.out -iw $IW -ih $IH -cr $ICW $ICH $ICW $ICH 0 -ow $OW -oh $OH -ol $OCW -ot $OCH -d $DISPLAY -r $ROT -vf $VF -hf $HF -v 0 -fr 60"
done
done
done
done
done
done
done

# user pointer test
run_testcase "./mxc_v4l2_output.out -iw $DISPW_DIV_2 -ih $DISPH_DIV_2 -ow $DISPW -oh $DISPH -d $DISPLAY -u"
run_testcase "./mxc_v4l2_output.out -iw $DISPW_DIV_2 -ih $DISPH_DIV_2 -ow $DISPW_DIV_2 -oh $DISPH_DIV_2 -d $DISPLAY -u"

else

# SDC output size test cases
if [ "$(platform)" != IMX25_3STACK ]; then
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
fi
fi

if [ $TEST_CAMERA = 1 ]; then

# V4L2 Capture Tests
if [ "$(platform)" = IMX25_3STACK ]; then
run_testcase "./csi_v4l2_overlay.out -t 10 -w 640 -h 480 -fr 30"
else
run_testcase "./mxc_v4l2_overlay.out -iw 640 -ih 480 -ow 240 -oh 320 -r 4 -fr 30 -fg -t 10"
run_testcase "./mxc_v4l2_overlay.out -iw 640 -ih 480 -ow 240 -oh 320 -r 4 -fr 30 -t 10"

for ROT in 0 1 2 3 4 5 6 7; do
	run_testcase "./mxc_v4l2_overlay.out -iw 640 -ih 480 -ow 240 -oh 184 -r $ROT -fr 30 -fg -t 5"
done

for POS in 0 4 8 16 32 64 128; do
	run_testcase "./mxc_v4l2_overlay.out -iw 640 -ih 480 -ot $POS -ol $POS -ow 80 -oh 60 -fr 30 -fg -t 5"
done
fi

fi


print_status

stop_time=`date`
echo =============== test start from $stop_time ==============================

exit $STATUS
