#ifndef NATS_CLIENT_H
#define NATS_CLIENT_H

#include <string>
#include <functional>

// NATS客户端封装类
class NatsClient {
public:
    // 构造函数
    NatsClient();
    // 析构函数
    ~NatsClient();

    // 连接到NATS服务器
    bool connect(const std::string& serverUrl);
    // 断开连接
    void disconnect();
    // 发布文本消息
    bool publish(const std::string& subject, const std::string& message);
    // 发布二进制数据（用于视频流）
    bool publishBinary(const std::string& subject, const uint8_t* data, size_t dataLen);
    // 订阅主题
    bool subscribe(const std::string& subject, std::function<void(const std::string&)> callback);
    // 取消订阅
    void unsubscribe(const std::string& subject);
    // 检查连接状态
    bool isConnected() const;

private:
    // NATS连接句柄
    void* m_natsConn;
    // 订阅句柄映射
    void* m_subscriptions;
    // 连接状态
    bool m_connected;
};

#endif // NATS_CLIENT_H