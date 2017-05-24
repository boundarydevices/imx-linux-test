#!/bin/bash

source /unit_tests/test-utils.sh

STATUS=0
if [[ $(platform) = i.MX6Q* ]] || [[ $(platform) = i.MX6D* ]]; then
	echo pxp_v4l2_out_test.sh not supported on current soc
	exit $STATUS
fi
./pxp_v4l2_test.out -sx 480 -sy 272 -res 352:240 -dst 0:0:352:240 -a 100 -w 2 fb-352x240.yuv  BLANK
./pxp_v4l2_test.out -sx 480 -sy 272 -res 352:240 -a 0 -r 90 fb-352x240.yuv  BLANK
./pxp_v4l2_test.out -sx 480 -sy 272 -res 352:240 -a 100 -o rgb24_file.s1 fb-352x240.yuv  BLANK
./pxp_v4l2_test.out -sx 480 -sy 272 -res 352:240 -a 100 -r 180 fb-352x240.yuv rgb24_file.s1
