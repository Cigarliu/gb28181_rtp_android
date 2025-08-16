LOCAL_PATH := $(call my-dir)

# 构建NATS静态库
include $(CLEAR_VARS)
LOCAL_MODULE := nats_static

# NATS库源文件
NATS_SRC_DIR := ../3rd/nats.c-3.10.1/src
LOCAL_SRC_FILES := $(NATS_SRC_DIR)/asynccb.c \
                   $(NATS_SRC_DIR)/buf.c \
                   $(NATS_SRC_DIR)/comsock.c \
                   $(NATS_SRC_DIR)/conn.c \
                   $(NATS_SRC_DIR)/crypto.c \
                   $(NATS_SRC_DIR)/dispatch.c \
                   $(NATS_SRC_DIR)/hash.c \
                   $(NATS_SRC_DIR)/js.c \
                   $(NATS_SRC_DIR)/jsm.c \
                   $(NATS_SRC_DIR)/kv.c \
                   $(NATS_SRC_DIR)/msg.c \
                   $(NATS_SRC_DIR)/nats.c \
                   $(NATS_SRC_DIR)/natstime.c \
                   $(NATS_SRC_DIR)/nkeys.c \
                   $(NATS_SRC_DIR)/nuid.c \
                   $(NATS_SRC_DIR)/opts.c \
                   $(NATS_SRC_DIR)/parser.c \
                   $(NATS_SRC_DIR)/pub.c \
                   $(NATS_SRC_DIR)/srvpool.c \
                   $(NATS_SRC_DIR)/stats.c \
                   $(NATS_SRC_DIR)/status.c \
                   $(NATS_SRC_DIR)/sub.c \
                   $(NATS_SRC_DIR)/timer.c \
                   $(NATS_SRC_DIR)/url.c \
                   $(NATS_SRC_DIR)/util.c \
                   $(NATS_SRC_DIR)/micro.c \
                   $(NATS_SRC_DIR)/micro_client.c \
                   $(NATS_SRC_DIR)/micro_endpoint.c \
                   $(NATS_SRC_DIR)/micro_error.c \
                   $(NATS_SRC_DIR)/micro_monitoring.c \
                   $(NATS_SRC_DIR)/micro_request.c \
                   $(NATS_SRC_DIR)/unix/sock.c \
                   $(NATS_SRC_DIR)/unix/thread.c \
                   $(NATS_SRC_DIR)/unix/cond.c \
                   $(NATS_SRC_DIR)/unix/mutex.c \
                   $(NATS_SRC_DIR)/glib/glib.c \
                   $(NATS_SRC_DIR)/glib/glib_async_cb.c \
                   $(NATS_SRC_DIR)/glib/glib_dispatch_pool.c \
                   $(NATS_SRC_DIR)/glib/glib_gc.c \
                   $(NATS_SRC_DIR)/glib/glib_last_error.c \
                   $(NATS_SRC_DIR)/glib/glib_ssl.c \
                   $(NATS_SRC_DIR)/glib/glib_timer.c

LOCAL_C_INCLUDES := $(NATS_SRC_DIR) \
                    $(NATS_SRC_DIR)/include \
                    $(NATS_SRC_DIR)/unix \
                    $(NATS_SRC_DIR)/glib

LOCAL_CFLAGS := -DNATS_STATIC
LOCAL_CPPFLAGS := -DNATS_STATIC

include $(BUILD_STATIC_LIBRARY)

# 构建主库
include $(CLEAR_VARS)

LOCAL_MODULE    := gb28181-lib
LOCAL_SRC_FILES := ../src/main/cpp/gb28181_jni.cpp \
                   ../src/main/cpp/nats_client.cpp \
                   ../src/main/cpp/nats_test.cpp \
                   ../src/main/cpp/rtp/RtpSendPs.cpp \
                   ../src/main/cpp/rtp/h264_streaming_framer.cpp

LOCAL_LDLIBS    := -llog

# 添加包含路径
LOCAL_C_INCLUDES := $(NDK_ROOT)/sources/cxx-stl/llvm-libc++/include \
                    $(NDK_ROOT)/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include \
                    $(NDK_ROOT)/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/android \
                    ../3rd/nats.c-3.10.1/src \
                    ../3rd/nats.c-3.10.1/src/include \
                    ../src/main/cpp \
                    ../src/main/cpp/rtp

LOCAL_STATIC_LIBRARIES := nats_static

include $(BUILD_SHARED_LIBRARY)