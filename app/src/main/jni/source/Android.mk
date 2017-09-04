LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL
IMGUI_PATH := ../imgui

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
     $(LOCAL_PATH)/$(IMGUI_PATH)

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	 my_volume_raycaster.cpp

LOCAL_SHARED_LIBRARIES := SDL2 imgui stb-image framework

# LOCAL_LDLIBS := -ldl -lGLESv1_CM -lGLESv2 -lGLESv3 -llog -landroid
LOCAL_LDLIBS := -ldl -lGLESv1_CM -lGLESv3 -llog -landroid

include $(BUILD_SHARED_LIBRARY)
