LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := imgui

SDL_PATH := ../SDL

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/*.cpp))

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -ldl -lGLESv1_CM -lGLESv3 -llog -landroid

include $(BUILD_SHARED_LIBRARY)