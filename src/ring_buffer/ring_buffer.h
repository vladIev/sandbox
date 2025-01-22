#pragma once
#include <vector>

using Timestamp = std::size_t;
template <std::size_t Limit, std::size_t Capacity>
class RingBuffer
{
  private:
	std::vector<Timestamp> d_buffer;
	Timestamp d_oldest = 0;
	std::size_t d_latest = 0;

  public:
	RingBuffer()
	{
		d_buffer.reserve(Capacity);
	}

	[[nodiscard]] bool push(const Timestamp value)
	{
		if(d_buffer.size() < Capacity)
		{
			d_buffer.push_back(value);
			d_oldest = value;
			d_latest++;
			return true;
		}

		if(value - d_oldest < Limit)
		{
			return false;
		}

		d_oldest = d_buffer[d_latest];
		d_latest = (d_latest + 1) % Capacity;
		d_buffer[d_latest] = value;
		return true;
	}

	[[nodiscard]] bool empty() const
	{
		return d_buffer.empty();
	}
};