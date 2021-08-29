#ifndef __THREAD_SAFE_QUEUE_HPP__
#define __THREAD_SAFE_QUEUE_HPP__

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <mutex>
#include <thread>
#include <list>
#include <assert.h>

template<typename T>
class ThreadSafeQueue {
public:
	ThreadSafeQueue() {};
	void Push(T pFrame) {
		mutex_.lock();
		list_.emplace_back(pFrame);
		mutex_.unlock();
	}
	T Pop() {
		T ret = NULL;
		if (!list_.empty()) {
			mutex_.lock();
			ret = list_.front();
			list_.pop_front();
			mutex_.unlock();
		}
		return ret;
	}
	int Size() {
		int ret = -1;
		mutex_.lock();
		ret = list_.size();
		mutex_.unlock();
		return ret;
	}
private:
	std::list<T> list_;
	std::mutex mutex_;
};

class ThreadSafeFlag {
public:
	void Set(bool value) {
		mutex_.lock();
		flag = value;
		mutex_.unlock();
	}
	bool Get() {
		bool ret = false;
		mutex_.lock();
		ret = flag;
		mutex_.unlock();
		return ret;
	}
private:
	bool flag = true;
	std::mutex mutex_;
};

#endif