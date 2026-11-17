LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := VHudRadioProbe64
LOCAL_SRC_FILES := VHudRadioProbe.cpp AndroidModLoader/mod/config.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/AndroidModLoader $(LOCAL_PATH)
LOCAL_CPPFLAGS := -std=c++17 -O2 -fno-exceptions -fno-rtti
LOCAL_LDLIBS := -llog -ldl
LOCAL_LDFLAGS := -Wl,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)
