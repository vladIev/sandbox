#include "spinlock.h"

#include <ranges>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

template <typename Spinlock>
void test_spinlock()
{
	Spinlock lock;
	uint64_t count = 0;
	std::vector<std::thread> threads;
	for(auto i : std::views::iota(0, 4))
	{

		threads.emplace_back([&]() {
			for(int i = 0; i < 10000; i++)
			{
				lock.lock();
				++count;
				lock.unlock();
			}
		});
	}

	for(auto& thread : threads)
	{
		thread.join();
	}

	EXPECT_EQ(count, 40000);
}

TEST(SpinlockTest, sp_naive)
{
	test_spinlock<SpinlockNaive>();
}

TEST(SpinlockTest, sp_with_proper_barriers)
{
	test_spinlock<SpinlockWithBarriers>();
}

TEST(SpinlockTest, sp_with_load)
{
	test_spinlock<SpinlockWithLoad>();
}

TEST(SpinlockTest, sp_final)
{
	test_spinlock<SpinlockWithBackoff>();
}

int main()
{
	testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}