#include "nats_client.h"
#include "rtp/RtpSendPs.h"
#include "rtp/h264_streaming_framer.h"
#include <android/log.h>
#include <cstdio>
#include <ctime>
#include <jni.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>

// 数据类型枚举（与Java层保持一致）
enum DataType {
  VIDEO_STREAM1 = 1,
  VIDEO_STREAM2 = 2,
  VIDEO_STREAM3 = 3,
  VIDEO_STREAM4 = 4,
  VIDEO_STREAM5 = 5,
  VIDEO_STREAM6 = 6,
  SIP_CONFIG1 = 7,
  SIP_CONFIG2 = 8,
  SIP_CONFIG3 = 9,
  SIP_CONFIG4 = 10,
  SIP_CONFIG5 = 11,
  SIP_CONFIG6 = 12
};

// SIP配置结构
struct SipConfig {
  std::string serverIp;
  int serverPort;
  int localPort;
  uint32_t ssrc;
  bool isValid;

  SipConfig() : serverPort(0), localPort(0), ssrc(0), isValid(false) {}

  bool operator==(const SipConfig &other) const {
    return serverIp == other.serverIp && serverPort == other.serverPort &&
           localPort == other.localPort && ssrc == other.ssrc;
  }

  bool operator!=(const SipConfig &other) const { return !(*this == other); }
};

// RTP实例管理结构
struct RtpInstance {
  std::unique_ptr<BXC::RtpSendPs> rtpSender;
  std::unique_ptr<Cigar::NALUSplitter> naluSplitter;
  std::unique_ptr<Cigar::ConcurrentQueueA<std::shared_ptr<Cigar::NALUData>>>
      naluQueue;
  SipConfig currentConfig;
  bool isRunning;

  RtpInstance() : isRunning(false) {
    naluQueue.reset(
        new Cigar::ConcurrentQueueA<std::shared_ptr<Cigar::NALUData>>());
  }
};

// 导入NATS测试函数
extern "C" {
void testNatsConnection(const char *serverUrl);
}

#define LOG_TAG "GB28181-JNI"
#define LOGI(...)                                                              \
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[GB28181-JNI] " __VA_ARGS__)
#define LOGE(...)                                                              \
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[GB28181-JNI] " __VA_ARGS__)
#define LOGD(...)                                                              \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[GB28181-JNI] " __VA_ARGS__)
#define LOGW(...)                                                              \
  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "[GB28181-JNI] " __VA_ARGS__)

// 全局NATS客户端实例
static NatsClient *g_natsClient = nullptr;
// NATS服务器地址
static std::string g_natsServer = "nats://localhost:4222";
// 保存目录路径
static std::string g_saveDirectory = "";

// RTP实例管理
static RtpInstance g_rtpInstances[6]; // 支持6路流
static std::mutex g_rtpMutex;         // 保护RTP实例的互斥锁

// 辅助函数：获取流索引（从DataType转换为数组索引）
static int getStreamIndex(int dataType) {
  if (dataType >= VIDEO_STREAM1 && dataType <= VIDEO_STREAM6) {
    return dataType - VIDEO_STREAM1; // 0-5
  }
  if (dataType >= SIP_CONFIG1 && dataType <= SIP_CONFIG6) {
    return dataType - SIP_CONFIG1; // 0-5
  }
  return -1; // 无效类型
}

// 辅助函数：解析SIP配置字符串
// 格式: "IP_端口_ssrc" (例如: 14.155.190.149_30478_200009003)
static bool parseSipConfig(const std::string &configStr, SipConfig &config) {
  size_t pos1 = configStr.find('_');
  if (pos1 == std::string::npos)
    return false;

  size_t pos2 = configStr.find('_', pos1 + 1);
  if (pos2 == std::string::npos)
    return false;

  try {
    config.serverIp = configStr.substr(0, pos1);
    config.serverPort = std::stoi(configStr.substr(pos1 + 1, pos2 - pos1 - 1));
    config.localPort = 0; // 本地端口设为0，让系统自动分配
    config.ssrc = std::stoul(configStr.substr(pos2 + 1));
    config.isValid = true;
    return true;
  } catch (const std::exception &e) {
    LOGE("Failed to parse SIP config: %s", e.what());
    return false;
  }
}

// 辅助函数：初始化或更新RTP实例
static bool initOrUpdateRtpInstance(int streamIndex,
                                    const SipConfig &newConfig) {
  std::lock_guard<std::mutex> lock(g_rtpMutex);

  RtpInstance &instance = g_rtpInstances[streamIndex];

  // 如果配置相同，无需更新
  if (instance.currentConfig.isValid && instance.currentConfig == newConfig) {
    LOGI("Stream %d: SIP config unchanged, skipping update", streamIndex + 1);
    return true;
  }

  // 如果实例正在运行，先停止
  if (instance.isRunning && instance.rtpSender) {
    instance.rtpSender->stop();
    instance.isRunning = false;
    LOGI("Stream %d: Stopped existing RTP sender for config update",
         streamIndex + 1);
  }

  // 创建新的RTP发送器
  try {
    instance.rtpSender.reset(
        new BXC::RtpSendPs(newConfig.serverIp.c_str(), newConfig.serverPort,
                           newConfig.localPort, instance.naluQueue.get()));

    // 创建NALU分割器
    instance.naluSplitter.reset(new Cigar::NALUSplitter(*instance.naluQueue));

    // 启动RTP发送器
    instance.rtpSender->start();
    instance.isRunning = true;
    instance.currentConfig = newConfig;

    LOGI("Stream %d: RTP sender initialized - %s:%d (local:%d, ssrc:%u)",
         streamIndex + 1, newConfig.serverIp.c_str(), newConfig.serverPort,
         newConfig.localPort, newConfig.ssrc);

    return true;
  } catch (const std::exception &e) {
    LOGE("Stream %d: Failed to initialize RTP sender: %s", streamIndex + 1,
         e.what());
    return false;
  }
}

// 辅助函数：处理视频流数据
static bool processVideoStream(int streamIndex, const uint8_t *data,
                               size_t dataLen) {
  // LOGD("Stream %d: Processing video data, size: %zu bytes", streamIndex + 1,
  // dataLen);

  std::lock_guard<std::mutex> lock(g_rtpMutex);

  RtpInstance &instance = g_rtpInstances[streamIndex];

  // 检查RTP实例是否已初始化
  if (!instance.currentConfig.isValid) {
    LOGE("Stream %d: RTP config not valid, dropping video data",
         streamIndex + 1);
    return false;
  }

  if (!instance.isRunning) {
    LOGE("Stream %d: RTP instance not running, dropping video data",
         streamIndex + 1);
    return false;
  }

  if (!instance.naluSplitter) {
    LOGE("Stream %d: NALU splitter not initialized, dropping video data",
         streamIndex + 1);
    return false;
  }

  // 打印数据头部信息用于调试
  // if (dataLen >= 4) {
  //     LOGD("Stream %d: Data header: 0x%02X 0x%02X 0x%02X 0x%02X",
  //          streamIndex + 1, data[0], data[1], data[2], data[3]);
  // }

  // 将数据推送到NALU分割器（分包处理，每次最多1400字节）
  try {
    const size_t maxPacketSize = 1400;
    size_t offset = 0;
    
    while (offset < dataLen) {
      size_t currentPacketSize = std::min(maxPacketSize, dataLen - offset);
      instance.naluSplitter->pushData(data + offset, currentPacketSize);
      
    //   LOGD("Stream %d: Successfully pushed packet %zu bytes (offset %zu/%zu) to NALU splitter",
    //        streamIndex + 1, currentPacketSize, offset, dataLen);
      
      offset += currentPacketSize;
    }
    
    // LOGD("Stream %d: Successfully pushed all %zu bytes in packets to NALU splitter",
    //      streamIndex + 1, dataLen);
    return true;
  } catch (const std::exception &e) {
    LOGE("Stream %d: Failed to process video data: %s", streamIndex + 1,
         e.what());
    return false;
  }
}

extern "C" {

// 示例JNI方法 - 返回一个简单的字符串
JNIEXPORT jstring JNICALL Java_com_example_gb28181jni_GB28181_stringFromJNI(
    JNIEnv *env, jobject /* this */) {
  std::string hello = "test version 2025.7.30 增加了拆包再分包的逻辑";
  return env->NewStringUTF(hello.c_str());
}

// 初始化GB28181相关功能并连接NATS服务器
JNIEXPORT jboolean JNICALL Java_com_example_gb28181jni_GB28181_initGB28181(
    JNIEnv *env, jobject /* this */, jstring saveDirectory) {
  return JNI_TRUE;
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
 // LOGI("Testing NATS connection...");
  testNatsConnection(g_natsServer.c_str());

  // 连接NATS服务器
  if (g_natsClient != nullptr && !g_natsClient->isConnected()) {
    if (g_natsClient->connect(g_natsServer)) {
      //LOGI("Connected to NATS server: %s", g_natsServer.c_str());
    } else {
      //LOGE("Failed to connect to NATS server: %s", g_natsServer.c_str());
      return JNI_FALSE;
    }
  }

  return JNI_TRUE;
}

// 输入数据到系统（多路RTP流处理）
JNIEXPORT jboolean JNICALL Java_com_example_gb28181jni_GB28181_inputData(
    JNIEnv *env, jobject /* this */, jint dataType, jbyteArray data) {
  // LOGI("=== inputData called: dataType=%d ===", dataType);

  // 获取流索引
  int streamIndex = getStreamIndex(dataType);
  if (streamIndex < 0) {
    LOGE("Invalid data type: %d", dataType);
    return JNI_FALSE;
  }

  // 获取数据
  jsize dataLen = env->GetArrayLength(data);
  jbyte *buffer = env->GetByteArrayElements(data, nullptr);



  // LOGI("Stream %d: Received data, type=%d, size=%d bytes", streamIndex + 1,
  // dataType, dataLen);

  bool result = false;

  // 根据数据类型处理
  if (dataType >= SIP_CONFIG1 && dataType <= SIP_CONFIG6) {
    // 处理SIP配置
    std::string configStr(reinterpret_cast<const char *>(buffer), dataLen);
    LOGI("Stream %d: Processing SIP config: %s", streamIndex + 1,
         configStr.c_str());

    SipConfig sipConfig;

    if (parseSipConfig(configStr, sipConfig)) {
      LOGI("Stream %d: SIP config parsed successfully - IP:%s, Port:%d, "
           "LocalPort:%d, SSRC:%u",
           streamIndex + 1, sipConfig.serverIp.c_str(), sipConfig.serverPort,
           sipConfig.localPort, sipConfig.ssrc);

      result = initOrUpdateRtpInstance(streamIndex, sipConfig);
      LOGI("Stream %d: SIP config processed - %s", streamIndex + 1,
           result ? "success" : "failed");
    } else {
      LOGE("Stream %d: Failed to parse SIP config: %s", streamIndex + 1,
           configStr.c_str());
    }
  } else if (dataType >= VIDEO_STREAM1 && dataType <= VIDEO_STREAM6) {
    // 处理视频流数据
    // LOGI("Stream %d: Processing video stream data", streamIndex + 1);

    result = processVideoStream(
        streamIndex, reinterpret_cast<const uint8_t *>(buffer), dataLen);

    if (result) {
      // LOGI("Stream %d: Video data processed successfully, pushed to RTP
      // pipeline", streamIndex + 1);
    } else {
      LOGE("Stream %d: Failed to process video data", streamIndex + 1);
    }

    // 可选：同时发布到NATS（保持原有功能）
    // if (g_natsClient != nullptr && g_natsClient->isConnected()) {
    //     std::string natsSubject = "video.stream." +
    //     std::to_string(streamIndex + 1);
    //     g_natsClient->publishBinary(natsSubject, reinterpret_cast<const
    //     uint8_t*>(buffer), dataLen); LOGD("Stream %d: Data also published to
    //     NATS subject: %s", streamIndex + 1, natsSubject.c_str());
    // } else {
    //     LOGD("Stream %d: NATS client not connected, skipping NATS publish",
    //     streamIndex + 1);
    // }

    // 可选：保存到文件（调试用）
    // if (!g_saveDirectory.empty() && result) {
    //     char filename[256];
    //     time_t now = time(0);
    //     sprintf(filename, "%s/stream%d_video_%ld.h264",
    //            g_saveDirectory.c_str(), streamIndex + 1, now);

    //     FILE* file = fopen(filename, "ab");  // 追加模式
    //     if (file) {
    //         fwrite(buffer, 1, dataLen, file);
    //         fclose(file);
    //         LOGD("Stream %d: Data saved to file: %s", streamIndex + 1,
    //         filename);
    //     } else {
    //         LOGW("Stream %d: Failed to save data to file: %s", streamIndex +
    //         1, filename);
    //     }
    // }
  } else {
    LOGE("Unknown data type: %d", dataType);
  }

  // 释放资源
  env->ReleaseByteArrayElements(data, buffer, JNI_ABORT);

  // LOGI("=== inputData finished: dataType=%d, result=%s ===", dataType, result
  // ? "SUCCESS" : "FAILED");

  return result ? JNI_TRUE : JNI_FALSE;
}

// 关闭GB28181客户端连接
JNIEXPORT jboolean JNICALL Java_com_example_gb28181jni_GB28181_closeGB28181(
    JNIEnv *env, jobject /* this */) {
  LOGI("Closing GB28181 client connection");

  // 停止所有RTP实例
  {
    std::lock_guard<std::mutex> lock(g_rtpMutex);
    for (int i = 0; i < 6; i++) {
      RtpInstance &instance = g_rtpInstances[i];
      if (instance.isRunning && instance.rtpSender) {
        instance.rtpSender->stop();
        instance.isRunning = false;
        LOGI("Stream %d: RTP sender stopped", i + 1);
      }

      // 清理资源
      instance.rtpSender.reset();
      instance.naluSplitter.reset();
      instance.currentConfig = SipConfig(); // 重置配置
    }
  }

  // 断开NATS连接
  if (g_natsClient != nullptr && g_natsClient->isConnected()) {
    g_natsClient->disconnect();
    LOGI("Disconnected from NATS server: %s", g_natsServer.c_str());
  }

  return JNI_TRUE;
}

// JNI_OnLoad函数，在库加载时调用
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  JNIEnv *env;
  if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
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
JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
  // 清理NATS客户端
  if (g_natsClient != nullptr) {
    delete g_natsClient;
    g_natsClient = nullptr;
    LOGI("NATS client destroyed in JNI_OnUnload");
  }
}

} // extern "C"