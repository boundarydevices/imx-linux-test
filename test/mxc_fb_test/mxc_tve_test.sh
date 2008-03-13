#!/bin/sh

echo Setting TV to NTSC mode
echo U:720x480i-60 > /sys/class/graphics/fb1/mode
echo 0 > /sys/class/graphics/fb1/blank
/unit_tests/mxc_v4l2_output.out -iw 720 -ih 480 -d 5
sleep 3

echo Blank the display
echo 3 > /sys/class/graphics/fb1/blank
sleep 1
echo Unblank the display
echo 0 > /sys/class/graphics/fb1/blank


echo Setting TV to PAL mode
echo U:720x576i-50 > /sys/class/graphics/fb1/mode
/unit_tests/mxc_v4l2_output.out -iw 720 -ih 576 -d 5
sleep 3

echo Blank the display
echo 3 > /sys/class/graphics/fb1/blank
sleep 1
echo Unblank the display
echo 0 > /sys/class/graphics/fb1/blank

echo 3 > /sys/class/graphics/fb1/blank

