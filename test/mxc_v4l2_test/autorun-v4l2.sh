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
# for camera
if [ $TEST_CAMERA = 1 ]; then
	if [ "$(platform)" = "IMX35_3STACK" ]; then
		modprobe ov2640_camera
		modprobe mxc_v4l2_capture
	fi
	if ([ "$(platform)" = IMX51 ] || [ "$(platform)" = IMX53 ] \
	|| [ "$(platform)" = IMX6 ]); then
		if [ "$(platform)" = "IMX51" ]; then
			modprobe ov3640_camera
		fi
		if [ "$(platform)" = "IMX53" ]; then
			modprobe ov5642_camera
		fi
		if [ "$(platform)" = "IMX6" ]; then
			modprobe ov5640_camera
		fi
		modprobe mxc_v4l2_capture
	fi
check_devnode "/dev/video0"
fi

# devnode test
# for display
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

if [ "$FULLTEST" = '1' ]; then
	CASES="176x144 320x240 640x480 720x480 720x576 800x600 1024x768 1024x800 1152x864 1280x720 1280x800 1280x1024 1366x768 1600x1200 1920x1080"
else
	CASES="1024x768"
fi

MODES=`cat /sys/class/graphics/fb0/modes`
if [ "$MODES" = "" ]; then
	MODES=`cat /sys/class/graphics/fb0/mode`
fi

for MODE in $MODES; do
	echo $MODE > /sys/class/graphics/fb0/mode
	echo Display in $MODE
	sleep 3
	DISPW=`cat /sys/class/graphics/fb0/mode | awk -F ':' '{print $2}' | awk -F 'x' '{print $1}'`
	DISPH=`cat /sys/class/graphics/fb0/mode | awk -F ':' '{print $2}' | awk -F 'x' '{print $2}' | awk -F 'p' '{print $1}'`
	for INPUT in $CASES; do
	for ROT in 0 90; do
	for VF in 0 1; do
	for HF in 0 1; do
		IW=`echo $INPUT | awk -F 'x' '{print $1}'`
		IH=`echo $INPUT | awk -F 'x' '{print $2}'`
		OW=$DISPW
		OH=$DISPH
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

DISPW_DIV_2=`expr $DISPW / 2`
DISPH_DIV_2=`expr $DISPH / 2`
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

for MODE in 0 1 2 3 4 5 6; do
	for RATO in 15 30; do
		if [ "$(platform)" = "IMX51" ]; then
			# ov3640_camera driver not support 30fps@mode3
			if ([ $MODE = 3 ] && [ $RATO = 30 ]); then
				continue
			fi

			if [ $MODE = 0 ]; then
				WIDTH=640
				HIGHT=480
			elif [ $MODE = 1 ]; then
				WIDTH=320
				HIGHT=240
			elif [ $MODE = 2 ]; then
				WIDTH=1024
				HIGHT=768
			elif [ $MODE = 3 ]; then
				WIDTH=2048
				HIGHT=1536
			elif [ $MODE = 4 ]; then
				WIDTH=720
				HIGHT=480
			elif [ $MODE = 5 ]; then
				WIDTH=720
				HIGHT=576
			else
				continue
			fi
		fi
		if [ "$(platform)" = "IMX53" ]; then
			if [ $RATO = 15 ]; then
				if ([ $MODE = 0 ] || [ $MODE = 1 ] || [ $MODE = 2 ] || [ $MODE = 3 ] || [ $MODE = 4 ]); then
					continue
				elif [ $MODE = 5 ]; then
					WIDTH=1920
					HIGHT=1080
				else
					WIDTH=2592
					HIGHT=1944
				fi
			fi
			if [ $RATO = 30 ]; then
				if [ $MODE = 0 ]; then
					WIDTH=640
					HIGHT=480
				elif [ $MODE = 1 ]; then
					WIDTH=320
					HIGHT=240
				elif [ $MODE = 2 ]; then
					WIDTH=720
					HIGHT=480
				elif [ $MODE = 3 ]; then
					WIDTH=720
					HIGHT=576
				elif [ $MODE = 4 ]; then
					WIDTH=1280
					HIGHT=720
				else
					continue
				fi
			fi
		fi
		if [ "$(platform)" = "IMX6" ]; then
			# ov5640_camera driver not support
			# 15fps@moder0 mode1 mode2 mode3 mode4 mode5
			# 30fps@mode6
			if ([ $MODE = 6 ] && [ $RATO = 30 ]); then
				continue
			fi

			if [ $RATO = 15 ]; then
				if ([ $MODE = 0 ] || [ $MODE = 1 ] || [ $MODE = 2 ] || [ $MODE = 3 ] || [ $MODE = 4 ] || [ $MODE = 5 ]); then
					continue
				fi
			fi

			if [ $MODE = 0 ]; then
				WIDTH=640
				HIGHT=480
			elif [ $MODE = 1 ]; then
				WIDTH=320
				HIGHT=240
			elif [ $MODE = 2 ]; then
				WIDTH=720
				HIGHT=480
			elif [ $MODE = 3 ]; then
				WIDTH=720
				HIGHT=576
			elif [ $MODE = 4 ]; then
				WIDTH=1280
				HIGHT=720
			elif [ $MODE = 5 ]; then
				WIDTH=1920
				HIGHT=1080
			else
				WIDTH=2592
				HIGHT=1944
			fi

		fi

		echo ==== v4l2 overlay base test: $WIDTH X $HIGHT @ $RATO ====
		for RUNTIME in 5 10 20 40 70 120; do
			echo ==== foreground runtime: $RUNTIME ====
			run_testcase "./mxc_v4l2_overlay.out -iw $WIDTH -ih $HIGHT -ow 640 -oh 480 -fr $RATO -m $MODE -t $RUNTIME -fg"
			echo ==== background runtime: $RUNTIME ====
			run_testcase "./mxc_v4l2_overlay.out -iw $WIDTH -ih $HIGHT -ow 640 -oh 480 -fr $RATO -m $MODE -t $RUNTIME"
		done

		echo ==== v4l2 overlay rotation test: $WIDTH X $HIGHT @ $RATO ====
		for ROT in 0 1 2 3 4 5 6 7; do
			echo ==== foreground rotation: $ROT ====
			run_testcase "./mxc_v4l2_overlay.out -iw $WIDTH -ih $HIGHT -ow 640 -oh 480 -fr $RATO -m $MODE -r $ROT -fg -t 5"
			echo ==== background rotation: $ROT ====
			run_testcase "./mxc_v4l2_overlay.out -iw $WIDTH -ih $HIGHT -ow 640 -oh 480 -fr $RATO -m $MODE -r $ROT -t 5"
		done

		echo ==== v4l2 overlay position test: $WIDTH X $HIGHT @ $RATO ====
		for POS in 0 4 8 16 32 64 128; do
			echo ==== foreground position: $POS ====
			run_testcase "./mxc_v4l2_overlay.out -iw $WIDTH -ih $HIGHT -ot $POS -ol $POS -ow 640 -oh 480 -fr $RATO -m $MODE -fg -t 5"
			echo ==== background position: $POS ====
			run_testcase "./mxc_v4l2_overlay.out -iw $WIDTH -ih $HIGHT -ot $POS -ol $POS -ow 640 -oh 480 -fr $RATO -m $MODE -t 5"
		done

		echo ==== v4l2 capture test: $WIDTH X $HIGHT @ $RATO ====
		for FCOUNT in 5 10 15 20 25 30; do
			echo ==== save frame count: $FCOUNT ====
			run_testcase "./mxc_v4l2_capture.out -$WIDTH -ih $HIGHT -ow 640 -oh 480 -fr $RATO -m $MODE -c $FCOUNT test.yuv"
		done
	done
done

fi

fi


print_status

stop_time=`date`
echo =============== test start from $stop_time ==============================

exit $STATUS
