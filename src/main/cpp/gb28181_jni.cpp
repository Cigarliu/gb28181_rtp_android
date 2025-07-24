#include <jni.h>
#include <string>
#include <android/log.h>
#include <ctime>
#include <cstdio>
#include "nats_client.h"

// 数据类型枚举（与Java层保持一致）
enum DataType {
    VIDEO_STREAM = 0,
    SIP_CONFIG = 1
};

// 导入NATS测试函数
extern "C" {
    void testNatsConnection(const char* serverUrl);
}

#define LOG_TAG "GB28181_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 全局NATS客户端实例
static NatsClient* g_natsClient = nullptr;
// NATS服务器地址
static std::string g_natsServer = "nats://localhost:4222";
// 默认主题
static std::string g_defaultSubjects[] = {
    "video.stream",  // VIDEO_STREAM
    "sip.config"     // SIP_CONFIG
};
// 保存目录路径
static std::string g_saveDirectory = "";

extern "C" {

// 示例JNI方法 - 返回一个简单的字符串
JNIEXPORT jstring JNICALL
Java_com_example_gb28181jni_GB28181_stringFromJNI(JNIEnv *env, jobject /* this */) {
    std::string hello = "test version 2025.7.23";
    return env->NewStringUTF(hello.c_str());
}

// 初始化GB28181相关功能并连接NATS服务器
JNIEXPORT jboolean JNICALL
Java_com_example_gb28181jni_GB28181_initGB28181(JNIEnv *env, jobject /* this */, 
                                              jstring saveDirectory) {
    LOGI("Initializing GB28181");
    
    // 使用写死的NATS服务器地址
    g_natsServer = "nats://cctv.mba:4222";
    
    // 如果提供了保存目录，则更新全局变量
    if (saveDirectory != nullptr) {
        const char *dirPath = env->GetStringUTFChars(saveDirectory, 0);
        g_saveDirectory = dirPath;
        LOGI("Set save directory: %s", g_saveDirectory.c_str());
        env->ReleaseStringUTFChars(saveDirectory, dirPath);
    } else {
        g_saveDirectory = ""; // 如果为null则不保存文件
        LOGI("No save directory specified, files will not be saved");
    }
    
    // 测试NATS连接
    LOGI("Testing NATS connection...");
    testNatsConnection(g_natsServer.c_str());
    
    // 连接NATS服务器
    if (g_natsClient != nullptr && !g_natsClient->isConnected()) {
        if (g_natsClient->connect(g_natsServer)) {
            LOGI("Connected to NATS server: %s", g_natsServer.c_str());
        } else {
            LOGE("Failed to connect to NATS server: %s", g_natsServer.c_str());
            return JNI_FALSE;
        }
    }
    
    return JNI_TRUE;
}

// 输入数据到系统（发送到NATS或保存到文件）
JNIEXPORT jboolean JNICALL
Java_com_example_gb28181jni_GB28181_inputData(JNIEnv *env, jobject /* this */,
                                           jint dataType, jbyteArray data) {
    // 检查NATS客户端是否已初始化并连接
    if (g_natsClient == nullptr || !g_natsClient->isConnected()) {
        LOGE("NATS client not initialized or not connected");
        return JNI_FALSE;
    }
    
    // 检查数据类型是否有效
    if (dataType < VIDEO_STREAM || dataType > SIP_CONFIG) {
        LOGE("Invalid data type: %d", dataType);
        return JNI_FALSE;
    }
    
    // 获取数据
    jsize dataLen = env->GetArrayLength(data);
    jbyte* buffer = env->GetByteArrayElements(data, nullptr);
    
    // 根据数据类型选择默认主题
    std::string pubSubject = g_defaultSubjects[dataType];
    
    bool result = false;
    
    // 如果是视频流类型且设置了保存目录，则保存到文件
    if (dataType == VIDEO_STREAM && !g_saveDirectory.empty()) {
        // 根据数据类型生成不同的文件名和扩展名
        char filename[256];
        time_t now = time(0);
        
        sprintf(filename, "%s/video_%ld.h264", g_saveDirectory.c_str(), now);
        
        // 保存文件
        FILE* file = fopen(filename, "wb");
        if (file) {
            size_t written = fwrite(buffer, 1, dataLen, file);
            fclose(file);
            result = (written == dataLen);
            LOGI("Saved video data to file: %s, size: %d bytes", filename, dataLen);
        } else {
            LOGE("Failed to open file for writing: %s", filename);
        }
    }
    
    // 无论是否保存文件，都发布到NATS
    bool publishResult = g_natsClient->publishBinary(pubSubject, reinterpret_cast<const uint8_t*>(buffer), dataLen);
    LOGI("Published data to NATS subject: %s, size: %d bytes", pubSubject.c_str(), dataLen);
    
    // 如果没有保存文件，则使用发布结果作为返回值
    if (!result) {
        result = publishResult;
    }
    
    // 释放资源
    env->ReleaseByteArrayElements(data, buffer, JNI_ABORT);
    
    return result ? JNI_TRUE : JNI_FALSE;
}

// 关闭GB28181客户端连接
JNIEXPORT jboolean JNICALL
Java_com_example_gb28181jni_GB28181_closeGB28181(JNIEnv *env, jobject /* this */) {
    LOGI("Closing GB28181 client connection");
    
    // 检查NATS客户端是否已初始化
    if (g_natsClient == nullptr) {
        LOGE("NATS client not initialized");
        return JNI_FALSE;
    }
    
    // 断开NATS连接
    if (g_natsClient->isConnected()) {
        g_natsClient->disconnect();
        LOGI("Disconnected from NATS server: %s", g_natsServer.c_str());
    }
    
    return JNI_TRUE;
}

// JNI_OnLoad函数，在库加载时调用
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    
    // 初始化NATS客户端
    if (g_natsClient == nullptr) {
        g_natsClient = new NatsClient();
        LOGI("NATS client created in JNI_OnLoad");
    }
    
    // 返回JNI版本
    return JNI_VERSION_1_6;
}

// JNI_OnUnload函数，在库卸载时调用
JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved) {
    // 清理NATS客户端
    if (g_natsClient != nullptr) {
        delete g_natsClient;
        g_natsClient = nullptr;
        LOGI("NATS client destroyed in JNI_OnUnload");
    }
}

} // extern "C"