#include <atomic>
#include <condition_variable>
#include <format>
#include <functional>
#include <iostream>
#include <mutex>
#include <new>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>

template <typename T, typename Alloc>
class MyVector
{
  private:
	Alloc allocator;
	T* data = nullptr;
	size_t capacity = 0;
	size_t size = 0;

	void resize()
	{
		const size_t new_capacity = std::max<size_t>(capacity * 2, 1u);

		T* newData = static_cast<T*>(operator new[](new_capacity * sizeof(T)));
		try
		{
			if constexpr(std::is_nothrow_move_constructible_v<T>)
			{
				for(size_t i = 0; i < size; i++)
				{
					new(&newData[i]) T(std::move(data[i]));
				}
			}
			else
			{
				for(size_t i = 0; i < size; i++)
				{
					new(&newData[i]) T(data[i]);
				}
			}
		}
		catch(...)
		{
			for(size_t i = 0; i < size; i++)
			{
				newData[i].~T();
			}
			operator delete[](newData);
			throw;
		}

		for(size_t i = 0; i < size; i++)
		{
			data[i].~T();
		}
		operator delete[](data);

		data = newData;
		capacity = new_capacity;
	}

  public:
	void push_back(const T& value)
	{
		if(size == capacity)
		{
			resize();
		}

		new(&data[size]) T(value);
		++size;
	}
};

auto main(int argc, char** argv) -> int
{

	return 0;
}