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

template<typename T>
class ThreadSafeQueue {
public:
	ThreadSafeQueue() {};
	void push(T pFrame) {
		mutex_.lock();
		list_.emplace_back(pFrame);
		mutex_.unlock();
	}
	T pop() {
		T ret = NULL;
		if (!list_.empty()) {
			mutex_.lock();
			ret = list_.front();
			list_.pop_front();
			mutex_.unlock();
		}
		return ret;
	}
	int size() {
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

#endif