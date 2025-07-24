# GB28181 JNI 与 NATS 集成

本项目实现了在 Android JNI 层接收视频流数据，并通过 NATS 消息系统发布出去的功能。

## 功能概述

- Java 层传入编码后的视频流数据到 JNI 层
- C++ 层通过 NATS 客户端将视频流数据发布到指定主题
- 支持自定义 NATS 服务器地址和发布主题

## 集成 NATS C 客户端库

要完成完整功能，您需要集成 NATS C 客户端库。以下是集成步骤：

### 1. 下载 NATS C 客户端库

```bash
git clone https://github.com/nats-io/nats.c.git
cd nats.c
```

### 2. 编译 NATS C 客户端库

#### 使用 CMake 编译

```bash
mkdir build
cd build
cmake -DNATS_BUILD_STREAMING=OFF -DNATS_BUILD_WITH_TLS=OFF ..
make
```

### 3. 将编译好的库文件和头文件添加到项目中

- 将编译生成的 `libnats.a` 或 `libnats.so` 复制到项目的 `src/main/jniLibs/arm64-v8a/` 目录下
- 将 NATS C 客户端库的头文件复制到项目的 `src/main/cpp/nats/` 目录下

### 4. 修改 CMakeLists.txt 文件

```cmake
# 添加 NATS 头文件路径
include_directories(src/main/cpp/nats)

# 添加 NATS 库文件
add_library(nats STATIC IMPORTED)
set_target_properties(nats PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libnats.a)

# 链接 NATS 库
target_link_libraries(
    gb28181-lib
    ${log-lib}
    nats
)
```

### 5. 修改 nats_client.cpp 文件

取消注释以下行，并添加实际的 NATS API 调用：

```cpp
// 取消注释这一行
#include "nats/nats.h"

// 在 connect 方法中使用实际的 NATS API
bool NatsClient::connect(const std::string& serverUrl) {
    // 实际的 NATS 连接代码
    natsStatus s = natsConnection_ConnectTo(&m_natsConn, serverUrl.c_str());
    m_connected = (s == NATS_OK);
    return m_connected;
}

// 在 publishBinary 方法中使用实际的 NATS API
bool NatsClient::publishBinary(const std::string& subject, const uint8_t* data, size_t dataLen) {
    if (!m_connected) return false;
    
    natsStatus s = natsConnection_Publish(m_natsConn, subject.c_str(), (const void*)data, dataLen);
    return (s == NATS_OK);
}
```

## 使用方法

### 在 Java 代码中使用

```java
// 初始化并连接 NATS 服务器
GB28181 gb28181 = new GB28181();
gb28181.initGB28181("192.168.1.100", 5060, "nats://your-nats-server:4222");

// 发送视频流数据
byte[] videoFrameData = getVideoFrameData(); // 获取视频帧数据
gb28181.sendVideoStream(videoFrameData, "video.stream.camera1");
```

### 自定义 NATS 主题

您可以在发送视频流时指定自定义主题，或者使用默认主题：

```java
// 使用自定义主题
gb28181.sendVideoStream(videoData, "custom.topic.name");

// 使用默认主题 ("video.stream")
gb28181.sendVideoStream(videoData, null);
```

## 注意事项

1. 当前实现是模拟的，需要集成实际的 NATS C 客户端库才能正常工作
2. 视频流数据应该是已编码的二进制数据（如 H.264/H.265 编码后的数据）
3. 确保 NATS 服务器可以从 Android 设备访问
4. 大型视频流可能需要考虑分片发送或使用 NATS JetStream 等持久化功能