#ifndef __THREAD_SAFE_FLAG_H__
#define __THREAD_SAFE_FLAG_H__

#include <mutex>
#include <thread>
#include <list>

class ThreadSafeFlag {
public:
	void set(bool value) {
		mutex_.lock();
		flag = value;
		mutex_.unlock();
	}
	bool get() {
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