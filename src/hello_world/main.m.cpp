#include <format>
#include <iostream>

auto main(int argc, char** argv) -> int
{
	std::cout << std::format("Hello world of C++ {}!\n", __cplusplus);
	return 0;
}