#pragma once
#include <atomic>
#include <ctime>

template <uint8_t LockImplementation = 0>
class Spinlock
{

	std::atomic_bool d_isLocked{false};

	void lock_sleep()
	{
		static const timespec ns = {0, 1};
		nanosleep(&ns, nullptr);
	}

	void unlock_impl(std::memory_order order)
	{
		d_isLocked.store(false, order);
	}

	void lock_impl(std::memory_order order)
	{
		while(d_isLocked.exchange(true, order))
			;
	}

	void lock_impl_with_load()
	{
		while(d_isLocked.load(std::memory_order_relaxed) ||
			  d_isLocked.exchange(true, std::memory_order_acquire))
			;
	}

	void lock_impl_final()
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

  public:
	void lock()
	{
		if constexpr(LockImplementation == 0)
		{
			lock_impl_final();
		}
		else if constexpr(LockImplementation == 1)
		{
			lock_impl(std::memory_order_acquire);
		}
		else if constexpr(LockImplementation == 2)
		{
			lock_impl_with_load();
		}
		else if constexpr(LockImplementation == 3)
		{
			lock_impl(std::memory_order_seq_cst);
		}
		else
		{
			static_assert(LockImplementation <= 3, "Invalid Spinlock implementation");
		}
	}

	void unlock()
	{
		if constexpr(LockImplementation != 3)
		{
			unlock_impl(std::memory_order_release);
		}
		else
		{
			unlock_impl(std::memory_order_seq_cst);
		}
	}
};
