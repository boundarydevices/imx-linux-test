
mxs_pxp_test.out -sx 480 -sy 272 -res 352:240 -dst 0:0:352:240 -a 100 -w 2 fb-352x240.yuv  BLANK
mxs_pxp_test.out -sx 480 -sy 272 -res 352:240 -a 0 -r 90 fb-352x240.yuv  BLANK
mxs_pxp_test.out -sx 480 -sy 272 -res 352:240 -a 100 -o rgb24_file.s1 fb-352x240.yuv  BLANK
mxs_pxp_test.out -sx 480 -sy 272 -res 352:240 -a 100 -r 180 fb-352x240.yuv rgb24_file.s1
