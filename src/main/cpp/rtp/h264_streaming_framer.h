#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <cstring>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <string>

#include "ConcurrentQueueA.h"


namespace Cigar {



	// 视频帧类型枚举
	enum class VideoFrameType {
		IDR = 5,        // I帧(关键帧)
		SEI = 6,        // SEI信息
		SPS = 7,        // 序列参数集
		PPS = 8,        // 图像参数集  
		NON_IDR = 1,    // P帧或B帧
		UNKNOWN = -1    // 未知类型
	};

	// 根据ID获取视频帧类型
	VideoFrameType getVideoFrameType(int id);

	// 判断是否为配置帧
	bool isConfigurationFrame(VideoFrameType type);

	// 获取帧类型名称
	const char* getFrameTypeName(VideoFrameType type);


	// NALU数据结构
	struct NALUData {
		std::vector<uint8_t> data;
		VideoFrameType type;
		std::chrono::system_clock::time_point timestamp;

		NALUData(const std::vector<uint8_t>& buffer, VideoFrameType frameType)
			: data(buffer), type(frameType), timestamp(std::chrono::system_clock::now()) {
		}

		// 获取不包含起始码的数据
		std::vector<uint8_t> getDataWithoutStartCode() const {
			if (data.size() < 3) return data;

			size_t startPos = 0;
			if (data.size() >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01) {
				startPos = 4;
			}
			else if (data.size() >= 3 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) {
				startPos = 3;
			}

			return std::vector<uint8_t>(data.begin() + startPos, data.end());
		}
	};

	// NALU分割器类声明
	class NALUSplitter {
	private:
		static const size_t NALU_MAXLEN = 1920 * 1080;
		static const uint8_t NALU_HEADER[4];

		std::vector<uint8_t> naluBuffer_;
		size_t naluDataIndex_;
		bool isSetKeyFrame_;
		ConcurrentQueueA<std::shared_ptr<NALUData>>& outputQueue_;

	public:
		explicit NALUSplitter(ConcurrentQueueA<std::shared_ptr<NALUData>>& queue);
		void pushData(const uint8_t* data, size_t size);
		void reset();

	private:
		bool isNALUHeader(const uint8_t* data);
		void processNALU(const std::vector<uint8_t>& buffer);
	};





}