#!/bin/bash

mkfifo /tmp/cap_pipe


width=72

echo "Testing Capture loopbacked to Video Output..."
while test $width -lt 240
do
    width=`expr $width + 8`
    height=`expr $width \* 480`
    height=`expr $height / 640`

    echo "Width =" $width
    echo "Height =" $height

    ./mxc_v4l2_output.out -iw $width -ih $height -ow $width -oh $height /tmp/cap_pipe &
    ./mxc_v4l2_capture.out -h $width -w $height -c 100 /tmp/cap_pipe
    sleep 1
done


width=72

echo "Testing Capture with Rotation loopbacked to Video Output..."
while test $width -lt 320
do
    width=`expr $width + 8`
    height=`expr $width \* 480`
    height=`expr $height / 640`
    remainder=`expr $height % 8`
    if test $remainder -le 3
    then
        height=`expr $height - $remainder`
    else
        remainder=`expr 8 - $remainder`
        height=`expr $height + $remainder`
    fi

    echo "Width =" $width
    echo "Height =" $height

    ./mxc_v4l2_output.out -ih $width -iw $height -oh $width -ow $height /tmp/cap_pipe &
    ./mxc_v4l2_capture.out -w $width -h $height -r 4 -c 100 /tmp/cap_pipe
    sleep 1
done

echo "Testing Capture loopbacked to Video Output with Rotation..."
width=72
while test $width -lt 320
do
    width=`expr $width + 8`
    height=`expr $width \* 480`
    height=`expr $height / 640`
    remainder=`expr $height % 8`
    if test $remainder -le 3
    then
        height=`expr $height - $remainder`
    else
        remainder=`expr 8 - $remainder`
        height=`expr $height + $remainder`
    fi

    echo "Width =" $width
    echo "Height =" $height

    ./mxc_v4l2_output.out -iw $width -ih $height -ow $height -oh $width -r 4 /tmp/cap_pipe &
    ./mxc_v4l2_capture.out -w $width -h $height -c 100 /tmp/cap_pipe
    sleep 1
done


rm /tmp/cap_pipe

