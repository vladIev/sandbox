#include "spinlock.h"

#include <benchmark/benchmark.h>

#include <atomic>
#include <mutex>
#include <thread>

int NUM_THREADS = std::thread::hardware_concurrency();

void BM_lock_mutex(benchmark::State& state)
{
	static size_t count = 0;
	if(state.thread_index() == 0)
	{
		count = 0;
	}
	static std::mutex mutex;
	for(auto _ : state)
	{
		std::lock_guard l(mutex);
		++count;
	}
}

BENCHMARK(BM_lock_mutex)->ThreadRange(2, NUM_THREADS)->UseRealTime();

void BM_lock_atomic(benchmark::State& state)
{
	static std::atomic<size_t> count = 0;
	if(state.thread_index() == 0)
	{
		count.store(0);
	}
	for(auto _ : state)
	{
		++count;
	}
}

BENCHMARK(BM_lock_atomic)->ThreadRange(2, NUM_THREADS)->UseRealTime();

void BM_lock_naive(benchmark::State& state)
{
	static size_t count = 0;
	if(state.thread_index() == 0)
	{
		count = 0;
	}

	static Spinlock<3> spinlock;
	for(auto _ : state)
	{
		spinlock.lock();
		++count;
		spinlock.unlock();
	}
}

BENCHMARK(BM_lock_naive)->ThreadRange(2, NUM_THREADS)->UseRealTime();

void BM_lock_with_proper_barriers(benchmark::State& state)
{
	static size_t count = 0;
	if(state.thread_index() == 0)
	{
		count = 0;
	}
	static Spinlock<1> spinlock;
	for(auto _ : state)
	{
		spinlock.lock();
		++count;
		spinlock.unlock();
	}
}

BENCHMARK(BM_lock_with_proper_barriers)->ThreadRange(2, NUM_THREADS)->UseRealTime();

void BM_lock_with_load(benchmark::State& state)
{
	static size_t count = 0;
	if(state.thread_index() == 0)
	{
		count = 0;
	}
	static Spinlock<2> spinlock;
	for(auto _ : state)
	{
		spinlock.lock();
		++count;
		spinlock.unlock();
	}
}

BENCHMARK(BM_lock_with_load)->ThreadRange(2, NUM_THREADS)->UseRealTime();

void BM_lock_final(benchmark::State& state)
{
	static size_t count = 0;
	if(state.thread_index() == 0)
	{
		count = 0;
	}
	static Spinlock<0> spinlock;
	for(auto _ : state)
	{
		spinlock.lock();
		++count;
		spinlock.unlock();
	}
}

BENCHMARK(BM_lock_final)->ThreadRange(2, NUM_THREADS)->UseRealTime();
