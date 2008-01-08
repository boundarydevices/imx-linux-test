#!/bin/sh
#
# Script to loopback MPEG4 encoded capture from camera to screen
#

mkfifo /tmp/fifo
/unit_tests/mxc_v4l2_test/mxc_v4l2_overlay.out -l 1 -t 5 -ow 176 -oh 144 &
/unit_tests/mxc_v4l2_test/mxc_v4l2_output.out -iw 352 -ih 288 -ow 176 -oh 144 -ot 160 -fg -d 0 /tmp/fifo &
sleep 1
/unit_tests/mxc_v4l2_test/mxc_v4l2_capture.out -w 352 -h 288 -c 300 -fr 30 /tmp/fifo

rm /tmp/fifo
