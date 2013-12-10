ifeq ($(BOARD_HAVE_VPU),true)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
       dec.c \
       capture.c \
       display.c \
       enc.c \
       fb.c \
       loopback.c \
       transcode.c \
       android_display.cpp \
       utils.c \
       main.c

LOCAL_CFLAGS += -DBUILD_FOR_ANDROID

LOCAL_C_INCLUDES += $(LOCAL_PATH) \
	external/linux-lib/ipu \
	external/linux-lib/vpu \
	device/fsl-proprietary/include

LOCAL_SHARED_LIBRARIES := libutils libc libvpu libipu libg2d libgui libui libhardware

LOCAL_MODULE := mxc-vpu-test
LOCAL_MODULE_TAGS := tests
include $(BUILD_EXECUTABLE)
endif
