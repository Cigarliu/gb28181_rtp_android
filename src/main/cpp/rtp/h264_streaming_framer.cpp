//
// Created by bxc on 2025/01/27
//

#include "h264_streaming_framer.h"
#include <iostream>
#include <cstring>
#include <android/log.h>

#define LOG_TAG "GB28181-JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[GB28181-JNI] " __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[GB28181-JNI] " __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[GB28181-JNI] " __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "[GB28181-JNI] " __VA_ARGS__)

namespace Cigar {

    // 根据ID获取视频帧类型
    VideoFrameType getVideoFrameType(int id) {
        switch (id) {
        case 7: return VideoFrameType::SPS;
        case 8: return VideoFrameType::PPS;
        case 5: return VideoFrameType::IDR;
        case 1: return VideoFrameType::NON_IDR;
        case 6: return VideoFrameType::SEI;
        default: return VideoFrameType::UNKNOWN;
        }
    }

    // 判断是否为配置帧
    bool isConfigurationFrame(VideoFrameType type) {
        return type == VideoFrameType::SPS ||
            type == VideoFrameType::PPS ||
            type == VideoFrameType::SEI;
    }

    // 获取帧类型名称
    const char* getFrameTypeName(VideoFrameType type) {
        switch (type) {
        case VideoFrameType::IDR: return "IDR";
        case VideoFrameType::SEI: return "SEI";
        case VideoFrameType::SPS: return "SPS";
        case VideoFrameType::PPS: return "PPS";
        case VideoFrameType::NON_IDR: return "NON_IDR";
        default: return "UNKNOWN";
        }
    }

    // NALUSplitter静态成员定义
    const uint8_t NALUSplitter::NALU_HEADER[4] = { 0, 0, 0, 1 };

    // NALUSplitter构造函数
    NALUSplitter::NALUSplitter(ConcurrentQueueA<std::shared_ptr<NALUData>>& queue)
        : naluBuffer_(NALU_MAXLEN), naluDataIndex_(0),
        isSetKeyFrame_(false), outputQueue_(queue) {
    }

    // 推送数据
    void NALUSplitter::pushData(const uint8_t* data, size_t size) {
        if (!data || size == 0) {
            LOGW("Invalid data pushed: data=%p, size=%zu", data, size);
            return;
        }

        LOGD("Pushing data to buffer: size=%zu, current buffer index=%zu", size, naluDataIndex_);
        
        // 打印数据头部信息
        if (size >= 4) {
            //LOGD("Data header: 0x%02X 0x%02X 0x%02X 0x%02X", data[0], data[1], data[2], data[3]);
        }

        size_t position = 0;

        while (position < size) {
            naluBuffer_[naluDataIndex_] = data[position];

            if (naluDataIndex_ == NALU_MAXLEN - 1) {
                LOGE("NALU buffer overflow");
                naluDataIndex_ = 0;
            }

            if (position + 4 < size) {
                if (isNALUHeader(&data[position])) {
                    if (naluDataIndex_ > 0) {
                        //LOGD("Found NALU: size=%zu bytes", naluDataIndex_);
                        std::vector<uint8_t> buffer(naluBuffer_.begin(),
                            naluBuffer_.begin() + naluDataIndex_);
                        processNALU(buffer);
                    }
                    naluDataIndex_ = 0;
                }
            }

            naluDataIndex_++;
            position++;
        }
        
        //LOGD("Data processing completed: final buffer index=%zu", naluDataIndex_);
    }

    // 重置
    void NALUSplitter::reset() {
        naluDataIndex_ = 0;
        isSetKeyFrame_ = false;
    }

    // 检查是否为NALU头
    bool NALUSplitter::isNALUHeader(const uint8_t* data) {
        return memcmp(data, NALU_HEADER, 4) == 0;
    }

    // 处理NALU
    void NALUSplitter::processNALU(const std::vector<uint8_t>& buffer) {
        if (buffer.empty() || buffer.size() < 5) {
            LOGW("NALU data too short: %zu bytes", buffer.size());
            return;
        }

        //LOGD("Processing NALU: total size=%zu bytes", buffer.size());

        int naluType = buffer[4] & 0x1F;
        VideoFrameType type = getVideoFrameType(naluType);

        if (type == VideoFrameType::UNKNOWN) {
            LOGW("Unknown NALU type: %d", naluType);
            return;
        }

        if (!isSetKeyFrame_) {
            if (type == VideoFrameType::IDR) {
                isSetKeyFrame_ = true;
                LOGI("Set key frame: %s", getFrameTypeName(type));
            }
            else {
                LOGD("Waiting for key frame, skip: %s", getFrameTypeName(type));
                return;
            }
        }

        auto naluData = std::make_shared<NALUData>(buffer, type);
        outputQueue_.push(naluData);
        //LOGI("Process NALU: %s, size: %zu bytes", getFrameTypeName(type), buffer.size());
    }

}