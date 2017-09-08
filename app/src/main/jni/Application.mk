
# Uncomment this if you're using STL in your project
# See CPLUSPLUS-SUPPORT.html in the NDK documentation for more information
# APP_STL := stlport_static
APP_STL := gnustl_static
APP_ABI := armeabi-v7a

# Min SDK level
APP_PLATFORM=android-21
APP_CPPFLAGS := -std=c++11 -fexceptions -Wall -Wformat
