#include "spinlock.h"

#include <format>
#include <iostream>
#include <mutex>
#include <ranges>
#include <thread>

auto main(int argc, char** argv) -> int
{
	Spinlock spinlock;
	auto m1 = [&](int id) {
		spinlock.lock();
		std::cout << std::format("Message from {}\n", id);
		spinlock.unlock();
	};

	auto t1 = std::thread([&]() {
		for(auto i : std::views::iota(0, 20))
		{
			m1(1);
		}
	});

	auto t2 = std::thread([&]() {
		for(auto i : std::views::iota(0, 20))
		{
			m1(2);
		}
	});

	t1.join();
	t2.join();

	return 0;
}