#!/bin/sh
#
# Script to loopback MPEG4 encoded capture from camera to screen
#

mkfifo /tmp/cap_pipe
mkfifo /tmp/mp4_pipe

./mxc_v4l2_overlay.out -ow 120 -oh 80 -ol 10 -ot $2 -t 50 -l 2 &
sleep 1
./mxc_mpeg4dec_test.out -d 0 /tmp/mp4_pipe &

if (test -r /root/argonplus) then
    ./cam2mpeg4 $1 $2 $3 30 /tmp/mp4_pipe
else
    ./mxc_mpeg4enc_test.out -w $1 -h $2 /tmp/cap_pipe /tmp/mp4_pipe &
    ./mxc_v4l2_capture.out -w $1 -h $2 -c $3 /tmp/cap_pipe
fi

rm /tmp/cap_pipe
rm /tmp/mp4_pipe

