#!/bin/bash

./mxc_v4l2_output.out -iw 352 -ih 240 -ow 176 -oh 120 -d 0 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 176 -oh 120 -d 0 -r 1 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 176 -oh 120 -d 0 -r 2 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 176 -oh 120 -d 0 -r 3 fb-352x240.yuv

./mxc_v4l2_output.out -iw 352 -ih 240 -ow 120 -oh 176 -d 0 -r 4 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 120 -oh 176 -d 0 -r 5 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 120 -oh 176 -d 0 -r 6 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 120 -oh 176 -d 0 -r 7 fb-352x240.yuv

./mxc_v4l2_output.out -iw 352 -ih 240 -ow 240 -oh 160 -d 3 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 240 -oh 160 -d 3 -r 1 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 240 -oh 160 -d 3 -r 2 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 240 -oh 160 -d 3 -r 3 fb-352x240.yuv

./mxc_v4l2_output.out -iw 352 -ih 240 -ow 160 -oh 240 -d 3 -r 4 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 160 -oh 240 -d 3 -r 5 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 160 -oh 240 -d 3 -r 6 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 160 -oh 240 -d 3 -r 7 fb-352x240.yuv



./mxc_v4l2_output.out -iw 352 -ih 240 -ow 64 -oh 40 -r 0 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 240 -oh 320 -r 0 fb-352x240.yuv
./mxc_v4l2_output.out -iw 352 -ih 240 -ow 240 -oh 320 -r 4 fb-352x240.yuv

./mxc_v4l2_output.out -iw 352 -ih 240 -ow 240 -oh 160 -f WBMP -r 1 fb-352x240.rgb

