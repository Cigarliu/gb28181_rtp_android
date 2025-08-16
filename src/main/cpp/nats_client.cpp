#include "nats_client.h"
#include <android/log.h>
#include <map>
#include <mutex>

// 包含NATS C客户端库的头文件
#include "nats/nats.h"

#define LOG_TAG "NATS_CLIENT"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 使用NATS C客户端库的实际实现

// 消息回调函数的包装器结构体
struct SubscriptionInfo {
    std::function<void(const std::string&)> callback;
};

// NATS消息回调函数
static void messageHandler(natsConnection *nc, natsSubscription *sub, natsMsg *msg, void *closure) {
    SubscriptionInfo* subInfo = (SubscriptionInfo*)closure;
    
    if (subInfo != nullptr && subInfo->callback) {
        const char* data = natsMsg_GetData(msg);
        if (data != nullptr) {
            subInfo->callback(std::string(data));
        }
    }
    
    // 释放消息
    natsMsg_Destroy(msg);
}

// 订阅管理类
class SubscriptionManager {
public:
    SubscriptionManager() {}
    
    ~SubscriptionManager() {
        // 清理所有订阅
        for (auto& pair : m_subscriptions) {
            if (pair.second.sub != nullptr) {
                natsSubscription_Destroy(pair.second.sub);
            }
            delete pair.second.info;
        }
        m_subscriptions.clear();
    }
    
    bool addSubscription(natsConnection* conn, const std::string& subject, 
                         std::function<void(const std::string&)> callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // 创建订阅信息
        SubscriptionInfo* subInfo = new SubscriptionInfo();
        subInfo->callback = callback;
        
        // 创建NATS订阅
        natsSubscription* sub = nullptr;
        natsStatus status = natsConnection_Subscribe(&sub, conn, subject.c_str(), messageHandler, subInfo);
        
        if (status != NATS_OK) {
            delete subInfo;
            return false;
        }
        
        // 保存订阅信息
        SubData data;
        data.sub = sub;
        data.info = subInfo;
        m_subscriptions[subject] = data;
        
        return true;
    }
    
    bool removeSubscription(const std::string& subject) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_subscriptions.find(subject);
        if (it != m_subscriptions.end()) {
            // 取消订阅
            if (it->second.sub != nullptr) {
                natsSubscription_Destroy(it->second.sub);
            }
            delete it->second.info;
            m_subscriptions.erase(it);
            return true;
        }
        
        return false;
    }
    
private:
    struct SubData {
        natsSubscription* sub;
        SubscriptionInfo* info;
    };
    
    std::map<std::string, SubData> m_subscriptions;
    std::mutex m_mutex;
};

NatsClient::NatsClient() : m_natsConn(nullptr), m_connected(false) {
    m_subscriptions = new SubscriptionManager();
    LOGI("NatsClient created");
    
    // 初始化NATS库
    natsStatus status = nats_Open(-1);
    if (status != NATS_OK) {
        LOGE("Failed to initialize NATS library: %s", nats_GetLastError(&status));
    }
}

NatsClient::~NatsClient() {
    disconnect();
    delete static_cast<SubscriptionManager*>(m_subscriptions);
    
    // 关闭NATS库
    nats_Close();
    
    LOGI("NatsClient destroyed");
}

bool NatsClient::connect(const std::string& serverUrl) {
    LOGI("Connecting to NATS server: %s", serverUrl.c_str());
    
    // 如果已经连接，先断开
    if (m_connected) {
        disconnect();
    }
    
    // 连接到NATS服务器
    natsStatus status = natsConnection_ConnectTo((natsConnection**)&m_natsConn, serverUrl.c_str());
    
    if (status != NATS_OK) {
        LOGE("Failed to connect to NATS server: %s", nats_GetLastError(&status));
        m_connected = false;
        return false;
    }
    
    m_connected = true;
    LOGI("Connected to NATS server");
    
    return m_connected;
}

void NatsClient::disconnect() {
    if (m_connected && m_natsConn != nullptr) {
        LOGI("Disconnecting from NATS server");
        
        // 销毁NATS连接
        natsConnection_Destroy((natsConnection*)m_natsConn);
        
        m_connected = false;
        m_natsConn = nullptr;
    }
}

bool NatsClient::publish(const std::string& subject, const std::string& message) {
    if (!m_connected || m_natsConn == nullptr) {
        LOGE("Cannot publish: not connected to NATS server");
        return false;
    }
    
    // 发布消息
    natsStatus status = natsConnection_PublishString(
        (natsConnection*)m_natsConn, subject.c_str(), message.c_str());
    
    if (status != NATS_OK) {
        LOGE("Failed to publish message: %s", nats_GetLastError(&status));
        return false;
    }
    
   // LOGI("Published message to subject '%s': %s", subject.c_str(), message.c_str());
    return true;
}

bool NatsClient::publishBinary(const std::string& subject, const uint8_t* data, size_t dataLen) {
    if (!m_connected || m_natsConn == nullptr) {
        LOGE("Cannot publish binary data: not connected to NATS server");
        return false;
    }
    
    // 发布二进制数据
    natsStatus status = natsConnection_Publish(
        (natsConnection*)m_natsConn, subject.c_str(), (const void*)data, dataLen);
    
    if (status != NATS_OK) {
        LOGE("Failed to publish binary data: %s", nats_GetLastError(&status));
        return false;
    }
    
   // LOGI("Published binary data (%zu bytes) to subject '%s'", dataLen, subject.c_str());
    return true;
}

bool NatsClient::subscribe(const std::string& subject, std::function<void(const std::string&)> callback) {
    if (!m_connected || m_natsConn == nullptr) {
        LOGE("Cannot subscribe: not connected to NATS server");
        return false;
    }
    
    // 使用SubscriptionManager添加订阅
    bool result = static_cast<SubscriptionManager*>(m_subscriptions)->addSubscription(
        (natsConnection*)m_natsConn, subject, callback);
    
    if (result) {
        //LOGI("Subscribed to subject: %s", subject.c_str());
    } else {
        LOGE("Failed to subscribe to subject: %s", subject.c_str());
    }
    
    return result;
}

void NatsClient::unsubscribe(const std::string& subject) {
    if (!m_connected) {
        LOGE("Cannot unsubscribe: not connected to NATS server");
        return;
    }
    
    // 使用SubscriptionManager移除订阅
    bool result = static_cast<SubscriptionManager*>(m_subscriptions)->removeSubscription(subject);
    
    if (result) {
        LOGI("Unsubscribed from subject: %s", subject.c_str());
    } else {
        LOGE("Subject not found or already unsubscribed: %s", subject.c_str());
    }
}

bool NatsClient::isConnected() const {
    return m_connected;
}