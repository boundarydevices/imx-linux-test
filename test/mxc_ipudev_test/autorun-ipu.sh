#!/bin/bash

source /unit_tests/test-utils.sh

start_time=`date`
echo =============== test start from $start_time ==============================

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0

# devnode test
if ([ "$(platform)" = IMX51 ] || [ "$(platform)" = IMX53 ] \
	|| [ "$(platform)" = IMX6 ]); then
check_devnode "/dev/mxc_ipu"
fi

# Turn off fb blanking
echo -e "\033[9;0]" > /dev/tty0

#
# IPU Tests
#

if ([ "$(platform)" = IMX51 ] || [ "$(platform)" = IMX53 ] \
	|| [ "$(platform)" = IMX6 ]); then
DISPW=`cat /sys/class/graphics/fb0/mode | awk -F ':' '{print $2}' | awk -F 'x' '{print $1}'`
DISPH=`cat /sys/class/graphics/fb0/mode | awk -F ':' '{print $2}' | awk -F 'x' '{print $2}' | awk -F 'p' '{print $1}'`
DISPW_DIV_2=`expr $DISPW / 2`
DISPW_DIV_4=`expr $DISPW / 4`
DISPH_DIV_2=`expr $DISPH / 2`
DISPH_DIV_4=`expr $DISPH / 4`

RGB_FULL_FILE=fullsize.rgbp
RGB_SMALL_FILE=smallsize.rgbp
YUV_FULL_FILE=fullsize.yuv
YUV_SMALL_FILE=smallsize.yuv
VDI_FILE=stefan_interlaced_320x240_5frames.yv12

# prepare files
rm -f $RGB_SMALL_FILE $RGB_FULL_FILE $YUV_SMALL_FILE $YUV_FULL_FILE
if ( [ "$DISPW" = 1024 ] && [ "$DISPH" = 768 ]); then
	cp wall-1024x768-565.rgb fullsize.rgbp
else
	run_testcase "./mxc_ipudev_test.out -c 1 -i 1024,768,RGBP,0,0,0,0,0,0 -O $DISPW,$DISPH,RGBP,0,0,0,0,0 -s 0 -f $RGB_FULL_FILE wall-1024x768-565.rgb"
fi
run_testcase "./mxc_ipudev_test.out -c 1 -i 1024,768,RGBP,0,0,0,0,0,0 -O 176,144,RGBP,0,0,0,0,0 -s 0 -f $RGB_SMALL_FILE wall-1024x768-565.rgb"
run_testcase "./mxc_ipudev_test.out -c 1 -i 1024,768,RGBP,0,0,0,0,0,0 -O $DISPW,$DISPH,UYVY,0,0,0,0,0 -s 0 -f $YUV_FULL_FILE wall-1024x768-565.rgb"
run_testcase "./mxc_ipudev_test.out -c 1 -i 1024,768,RGBP,0,0,0,0,0,0 -O 176,144,UYVY,0,0,0,0,0 -s 0 -f $YUV_SMALL_FILE wall-1024x768-565.rgb"

if [ "$FULLTEST" = '1' ]; then
OWCASE="$DISPW_DIV_4 $DISPW_DIV_2 $DISPW"
OHCASE="$DISPH_DIV_4 $DISPH_DIV_2 $DISPH"
OROTCASE="0 1 2 3 4 5 6 7"
else
OWCASE="$DISPW"
OHCASE="$DISPH"
OROTCASE="0 1 4"
fi

echo =========== do test on display $DISPW $DISPH ====================
for FILE in $RGB_SMALL_FILE $RGB_FULL_FILE $YUV_SMALL_FILE $YUV_FULL_FILE $VDI_FILE; do
for OW in $OWCASE; do
for OH in $OHCASE; do
for OROT in $OROTCASE; do
for OFMT in RGBP RGB3 UYVY I420; do
LOOP=150
FCN=1
VDI=0
if ([ "$FILE" = "$RGB_SMALL_FILE" ] || [ "$FILE" = "$RGB_FULL_FILE" ]); then
IFMT=RGBP
fi
if ([ "$FILE" = "$YUV_SMALL_FILE" ] || [ "$FILE" = "$YUV_FULL_FILE" ]); then
IFMT=UYVY
fi

if [ "$FILE" = "$VDI_FILE" ]; then
IW=320
IH=240
IFMT=YV12
LOOP=10
FCN=4
VDI=1
fi

if ([ "$FILE" = "$RGB_SMALL_FILE" ] || [ "$FILE" = "$YUV_SMALL_FILE" ]); then
IW=176
IH=144
fi
if ([ "$FILE" = "$RGB_FULL_FILE" ] || [ "$FILE" = "$YUV_FULL_FILE" ]); then
IW=$DISPW
IH=$DISPH
fi

ICW=`expr $IW / 4`
ICH=`expr $IH / 4`
OCW=`expr $OW / 4`
OCH=`expr $OH / 4`
OW=`expr $OW - $OW % 8`
OH=`expr $OH - $OH % 8`
OCW=`expr $OCW - $OCW % 8`
OCH=`expr $OCH - $OCH % 8`

if [ "$VDI" = 1 ]; then
	#low motion test
	run_testcase "./mxc_ipudev_test.out -c $FCN -l $LOOP -i $IW,$IH,$IFMT,0,0,0,0,1,0 -O $OW,$OH,$OFMT,$OROT,0,0,0,0 -s 1 $FILE"
	#high motion test
	run_testcase "./mxc_ipudev_test.out -c $FCN -l $LOOP -i $IW,$IH,$IFMT,0,0,0,0,1,2 -O $OW,$OH,$OFMT,$OROT,0,0,0,0 -s 1 $FILE"
	cat /dev/zero > /dev/fb1
	# crop test
	run_testcase "./mxc_ipudev_test.out -c $FCN -l $LOOP -i $IW,$IH,$IFMT,$ICW,$ICH,$ICW,$ICH,1,0 -O $OW,$OH,$OFMT,$OROT,$OCW,$OCH,$OCW,$OCH -s 1 $FILE"
else
	# normal case
	run_testcase "./mxc_ipudev_test.out -c $FCN -l $LOOP -i $IW,$IH,$IFMT,0,0,0,0,0,0 -O $OW,$OH,$OFMT,$OROT,0,0,0,0 -s 1 $FILE"
	# overlay + crop test
	# ipudev unit test fill non-interleaved format to overlay with incorrect data, so ignore such test
	if ([ "$OFMT" = I420 ] || [ "$OFMT" = NV12 ] || [ "$OFMT" = YV12 ]); then
		OV_EN=0
	else
		OV_EN=1
	fi
	if ([ "$OROT" = 0 ] || [ "$OROT" = 1 ] || [ "$OROT" = 2 ] || [ "$OROT" = 3 ]); then
		OVW=$OCW
		OVH=$OCH
	else
		OVW=$OCH
		OVH=$OCW
	fi
	cat /dev/zero > /dev/fb1
	# test with global alpha
	run_testcase "./mxc_ipudev_test.out -c $FCN -l $LOOP -i $IW,$IH,$IFMT,$ICW,$ICH,$ICW,$ICH,0,0 -o $OV_EN,$OVW,$OVH,$OFMT,0,0,0,0,0,128,1,0xffffff -O $OW,$OH,$OFMT,$OROT,$OCW,$OCH,$OCW,$OCH -s 1 $FILE"
	# test with local alpha
	run_testcase "./mxc_ipudev_test.out -c $FCN -l $LOOP -i $IW,$IH,$IFMT,$ICW,$ICH,$ICW,$ICH,0,0 -o $OV_EN,$OVW,$OVH,$OFMT,0,0,0,0,1,128,1,0xffffff -O $OW,$OH,$OFMT,$OROT,$OCW,$OCH,$OCW,$OCH -s 1 $FILE"
fi
done
done
done
done
done

rm -f $RGB_SMALL_FILE $RGB_FULL_FILE $YUV_SMALL_FILE $YUV_FULL_FILE

fi

print_status

stop_time=`date`
echo =============== test stop at $stop_time ==============================

exit $STATUS
