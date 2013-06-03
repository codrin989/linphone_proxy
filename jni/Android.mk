# A simple test for the minimal standard C library
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES = $(LOCAL_PATH)/includes
LOCAL_CFLAGS = -Wextra
LOCAL_MODULE := linphone_proxy
LOCAL_SRC_FILES := main.c utils.c socket.c init.c run_proxy.c
include $(BUILD_EXECUTABLE)
