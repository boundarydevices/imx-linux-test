ifeq ($(BOARD_SOC_CLASS),IMX6)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
       memtool.c \
       mx6dl_modules.c \
       mx6q_modules.c \
       mx6sl_modules.c

#LOCAL_CFLAGS += -DBUILD_FOR_ANDROID

LOCAL_C_INCLUDES += $(LOCAL_PATH) \


LOCAL_SHARED_LIBRARIES := libutils libc

LOCAL_MODULE := memtool
LOCAL_MODULE_TAGS := tests
include $(BUILD_EXECUTABLE)
endif
