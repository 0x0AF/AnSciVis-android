LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := framework

SDL_PATH := ../SDL
IMGUI_PATH := ../imgui

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
     $(LOCAL_PATH)/$(IMGUI_PATH)

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/*.cpp))

LOCAL_SHARED_LIBRARIES := SDL2 imgui stb-image

# LOCAL_LDLIBS := -ldl -lGLESv1_CM -lGLESv2 -lGLESv3 -llog -landroid
LOCAL_LDLIBS := -ldl -lGLESv1_CM -lGLESv3 -llog -landroid

include $(BUILD_SHARED_LIBRARY)
