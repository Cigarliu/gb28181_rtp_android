#include "nats_client.h"
#include <android/log.h>

#define LOG_TAG "NATS_TEST"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 测试NATS客户端功能
extern "C" {

// 测试NATS连接和发布
void testNatsConnection(const char* serverUrl) {
    LOGI("Testing NATS connection to %s", serverUrl);
    
    // 创建NATS客户端
    NatsClient client;
    
    // 连接到NATS服务器
    if (client.connect(serverUrl)) {
       // LOGI("NATS connection successful");
        
        // 发布测试消息
        if (client.publish("test.subject", "Hello from GB28181 JNI")) {
         //   LOGI("Test message published successfully");
        } else {
          //  LOGE("Failed to publish test message");
        }
        
        // 断开连接
        client.disconnect();
        //LOGI("NATS connection closed");
    } else {
        //LOGE("Failed to connect to NATS server");
    }
}

} // extern "C"