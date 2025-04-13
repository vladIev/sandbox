#pragma once
#include <atomic>
#include <ctime>

class SpinlockNaive
{
	std::atomic_bool d_isLocked{false};

  public:
	void lock()
	{
		while(d_isLocked.exchange(true))
			;
	}

	void unlock()
	{
		d_isLocked.store(false);
	}
};

class SpinlockWithLoad
{
	std::atomic_bool d_isLocked{false};

  public:
	void lock()
	{
		while(d_isLocked.load() || d_isLocked.exchange(true))
			;
	}

	void unlock()
	{
		d_isLocked.store(false);
	}
};

class SpinlockWithBarriers
{
	std::atomic_bool d_isLocked{false};

  public:
	void lock()
	{
		while(d_isLocked.load(std::memory_order_relaxed) ||
			  d_isLocked.exchange(true, std::memory_order_acquire))
			;
	}

	void unlock()
	{
		d_isLocked.store(false, std::memory_order_release);
	}
};

class SpinlockWithBackoff
{
	std::atomic_bool d_isLocked{false};

	void lock_sleep()
	{
		static const timespec ns = {0, 1};
		nanosleep(&ns, nullptr);
	}

  public:
	void lock()
	{
		for(int i = 0; d_isLocked.load(std::memory_order_relaxed) ||
					   d_isLocked.exchange(true, std::memory_order_acquire);
			i++)
		{
			if(i == 128)
			{
				lock_sleep();
				i = 0;
			}
		}
	}

	void unlock()
	{
		d_isLocked.store(false, std::memory_order_release);
	}
};
