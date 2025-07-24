# GB28181 JNI 与 NATS 集成指南

本文档说明了如何在GB28181 JNI项目中集成NATS消息系统，实现在C++层接收各类数据（视频流和SIP配置信息）并通过NATS发布出去，同时支持将视频流数据保存到本地文件的功能。

## 已完成的集成

项目已经成功集成了NATS C客户端库，主要完成了以下工作：

1. 添加了NATS C客户端库的源代码（位于`3rd/nats.c-3.10.1`目录）
2. 修改了`CMakeLists.txt`文件，添加了NATS库的编译和链接配置
3. 实现了`NatsClient`类，封装了NATS的连接、发布、订阅等功能
4. 在JNI层提供了通用数据输入接口，支持多种数据类型（视频流和SIP配置信息）

## 使用方法

### 在Java层调用

```java
// 初始化并连接NATS服务器，同时设置视频流保存目录
GB28181 gb28181 = new GB28181();
String saveDirectory = "/storage/emulated/0/DCIM/Camera"; // 指定视频流保存目录，如果为null则不保存文件
gb28181.initGB28181(saveDirectory);

// 发送视频流数据（会同时发布到NATS服务器并保存到本地文件）
byte[] videoFrameData = getVideoFrameData(); // 获取视频帧数据
gb28181.inputData(GB28181.DataType.VIDEO_STREAM, videoFrameData);

// 发送SIP配置信息（只发布到NATS服务器）
byte[] sipConfigData = getSipConfigData(); // 获取SIP配置信息
gb28181.inputData(GB28181.DataType.SIP_CONFIG, sipConfigData);

// 使用完毕后关闭连接
gb28181.closeGB28181();
```

### 使用示例类

项目中提供了`NatsExample`类，演示了如何在Android应用中使用GB28181 JNI接口：

```java
// 创建示例类实例
NatsExample example = new NatsExample();

// 初始化（内部会设置视频流保存目录）
example.initialize();

// 发送各类数据
byte[] videoData = getVideoData(); // 获取视频数据
example.inputData(GB28181.DataType.VIDEO_STREAM, videoData);

byte[] sipConfigData = getSipConfigData(); // 获取SIP配置信息
example.inputData(GB28181.DataType.SIP_CONFIG, sipConfigData);

// 使用完毕后关闭
example.close();
```

### 数据类型和保存机制

系统内部会根据数据类型自动选择合适的NATS主题进行发布。同时，对于视频流类型的数据，如果在初始化时设置了保存目录，系统会自动将视频流保存到本地文件：

```java
// 在初始化时设置视频流保存目录
String saveDirectory = "/storage/emulated/0/DCIM/Camera"; // 如果为null则不保存文件
gb28181.initGB28181(saveDirectory);

// 发送数据（系统会根据数据类型自动处理）
gb28181.inputData(GB28181.DataType.VIDEO_STREAM, videoData); // 会同时发布到NATS和保存到本地
gb28181.inputData(GB28181.DataType.SIP_CONFIG, sipConfigData); // 只发布到NATS
```

视频流数据保存为本地文件时，使用以下命名格式：
- 视频流：`video_时间戳.h264`

## 编译说明

项目使用CMake构建系统，已经配置好了NATS库的编译选项。编译步骤如下：

1. 确保已安装Android NDK（项目使用NDK版本26.1.10909125）
2. 运行`build_cmake_en.bat`脚本进行编译
3. 编译成功后，输出文件位于`build/`目录下

## NATS库配置说明

在`CMakeLists.txt`文件中，我们对NATS库进行了以下配置：

```cmake
# NATS库设置
set(NATS_DIR ${CMAKE_SOURCE_DIR}/3rd/nats.c-3.10.1)
set(NATS_BUILD_WITH_TLS OFF)  # 禁用TLS支持
set(NATS_BUILD_EXAMPLES OFF)   # 不编译示例
set(NATS_BUILD_STREAMING OFF)  # 不编译Streaming支持
```

如果需要启用TLS支持或其他功能，可以修改上述配置。

## 注意事项

1. 输入的数据应该是已编码的二进制数据：
   - 视频流：H.264/H.265编码后的数据
   - SIP配置信息：通常为JSON格式的字节数组
2. 确保NATS服务器可以从Android设备访问
3. 大型数据流可能需要考虑分片发送或使用NATS JetStream等持久化功能
4. 当前实现使用了NATS的静态库，如果需要使用动态库，需要修改CMakeLists.txt文件
5. 保存视频流文件到本地时，请确保应用具有相应的存储权限
6. 在Android 10（API级别29）及以上版本，请使用应用专属存储或通过SAF（Storage Access Framework）获取的路径
7. 保存的视频文件格式为原始H.264/H.265数据，可能需要额外处理才能在播放器或其他应用中使用
8. 只有视频流类型的数据会保存到本地文件，SIP配置信息只会发布到NATS服务器

## 故障排除

如果遇到编译或运行问题，请检查：

1. NDK路径是否正确
2. NATS服务器地址是否可访问
3. 日志输出中是否有NATS相关的错误信息
4. 保存文件时，检查应用是否具有存储权限
5. 检查指定的目录路径是否存在且可写
6. 如果文件无法保存，检查Android日志中的错误信息