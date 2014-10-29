
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES = mmdc.c
LOCAL_MODULE := mmdc
LOCAL_MODULE_TAGS := 	optional eng
LOCAL_C_INCLUDES :=	mmdc.h

LOCAL_CFLAGS += \
        -Wall \
        -O3 \
        -fsigned-char \
        -DANDROID

include $(BUILD_EXECUTABLE)

