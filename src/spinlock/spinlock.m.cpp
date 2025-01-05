#include <atomic>
#include <iostream>
#include <mutex>

class Spinlock
{

	std::mutex d_lock;

  public:
	void lock()
	{
		d_lock.lock();
	}
	void unlock()
	{
		d_lock.unlock();
	}
};

auto main(int argc, char** argv) -> int
{
	Spinlock spinlock;
	auto t1ь1 = [&]() {
		spinlock.lock();
		std::cout << "Message 1" << std::endl;
		spinlock.unlock();
	};

	auto t2 = [&]() {
		spinlock.lock();
		std::cout << "Message 2" << std::endl;
		spinlock.unlock();
	};

	return 0;
}