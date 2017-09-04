LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := source

SDL_PATH := ../SDL
IMGUI_PATH := ../imgui

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
     $(LOCAL_PATH)/$(IMGUI_PATH)

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	 main.cpp

LOCAL_SHARED_LIBRARIES := SDL2 imgui

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv3 -llog

include $(BUILD_SHARED_LIBRARY)
