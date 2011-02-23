#!/bin/bash

source /unit_tests/test-utils.sh

#
# Exit status is 0 for PASS, nonzero for FAIL
#
STATUS=0
test_data_path=/unit_tests/data/pf

# For debugging purposes (for the log), check to see if test data is there.
ls -lh $test_data_path

# devnode test
check_devnode "/dev/mxc_ipu_pf"

#
# PF MPEG4 Tests
#
#run_testcase "./mxc_pf_test.out -w 352 -h 240 -m 1 $test_data_path/mpeg4_in.yuv $test_data_path/mpeg4_in.qp $test_data_path/mpeg4_out_deblock.yuv"
#run_testcase "./mxc_pf_test.out -w 352 -h 240 -m 2 $test_data_path/mpeg4_in.yuv $test_data_path/mpeg4_in.qp $test_data_path/mpeg4_out_dering.yuv"
run_testcase "./mxc_pf_test.out -w 352 -h 240 -m 3 $test_data_path/mpeg4_in.yuv $test_data_path/mpeg4_in.qp $test_data_path/mpeg4_out_dering_deblock.yuv"
run_testcase "./mxc_pf_test.out -w 352 -h 240 -m 3 -async $test_data_path/mpeg4_in.yuv $test_data_path/mpeg4_in.qp $test_data_path/mpeg4_out_dering_deblock.yuv"

#
# PF H.264 Tests
#
run_testcase "./mxc_pf_test.out -w 176 -h 144 -m 4 $test_data_path/h264_in.yuv $test_data_path/h264_in.qp $test_data_path/h264_out.yuv"

print_status
exit $STATUS
