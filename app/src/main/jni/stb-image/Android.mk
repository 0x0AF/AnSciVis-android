LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := stb-image

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/*.c))

LOCAL_LDLIBS := -ldl -llog -landroid

include $(BUILD_SHARED_LIBRARY)
