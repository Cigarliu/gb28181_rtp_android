LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := gb28181-lib
LOCAL_SRC_FILES := ../src/main/cpp/gb28181_jni.cpp
LOCAL_LDLIBS    := -llog

# 添加包含路径
LOCAL_C_INCLUDES := $(NDK_ROOT)/sources/cxx-stl/llvm-libc++/include \
                    $(NDK_ROOT)/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include \
                    $(NDK_ROOT)/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/android

include $(BUILD_SHARED_LIBRARY)