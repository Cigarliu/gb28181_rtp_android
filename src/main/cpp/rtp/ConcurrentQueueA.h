#pragma once
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>


namespace Cigar {


	// Thread-safe queue for raw bytes or AVPackets
	template<typename T>
	class ConcurrentQueueA {
	private:
		std::queue<T> queue_;
		mutable std::mutex mutex_;
		std::condition_variable cond_;
		size_t max_capacity_ = 2048 * 1024;
		bool exit_ = false;

	public:
		ConcurrentQueueA() {}

		void push(const T& value) {
			std::lock_guard<std::mutex> lock(mutex_);
			if (queue_.size() >= max_capacity_) queue_.pop();
			queue_.push(value);
			cond_.notify_one();
		}

		void push(const T* data, size_t size) {
			std::lock_guard<std::mutex> lock(mutex_);
			for (size_t i = 0; i < size; ++i) {
				if (queue_.size() >= max_capacity_) queue_.pop();
				queue_.push(data[i]);
			}
			cond_.notify_one();
		}

		bool pop(T& value, int timeout_ms = -1) {
			std::unique_lock<std::mutex> lock(mutex_);
			if (timeout_ms < 0) {
				cond_.wait(lock, [this] { return exit_ || !queue_.empty(); });
			}
			else {
				if (!cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
					[this] { return exit_ || !queue_.empty(); })) {
					return false; // 超时
				}
			}
			if (queue_.empty()) return false;
			value = queue_.front();
			queue_.pop();
			return true;
		}

		int pop(T* data, size_t size) {
			std::unique_lock<std::mutex> lock(mutex_);
			cond_.wait(lock, [this] { return exit_ || !queue_.empty(); });
			if (queue_.empty()) return -1;
			// Manually compute minimum to avoid std::min conflicts
			size_t read_size = queue_.size() < size ? queue_.size() : size;
			for (size_t i = 0; i < read_size; ++i) {
				data[i] = queue_.front();
				queue_.pop();
			}
			return static_cast<int>(read_size);
		}

		void exit() {
			std::lock_guard<std::mutex> lock(mutex_);
			exit_ = true;
			cond_.notify_all();
		}

		bool is_exit() const {
			std::lock_guard<std::mutex> lock(mutex_);
			return exit_;
		}

		// 更新capacity 不会立即清空队列，而是等待所有数据被处理完
		void set_max_capacity(size_t capacity) {
			std::lock_guard<std::mutex> lock(mutex_);
			max_capacity_ = capacity;
		}


		// 获取当前队列大小
		size_t size() const {
			std::lock_guard<std::mutex> lock(mutex_);
			return queue_.size();
		}
	};

}