#include "ring_buffer.h"

#include <gtest/gtest.h>

TEST(buffer, init)
{
	RingBuffer<5, 10> buffer;
	EXPECT_TRUE(buffer.empty());
}

TEST(buffer, push)
{
	const Timestamp limit = 10;

	RingBuffer<10, limit> buffer;
	for(int i = 0; i < limit; i++)
	{
		EXPECT_TRUE(buffer.push(limit + i));
	}
	EXPECT_FALSE(buffer.push(2 * limit));
	EXPECT_TRUE(buffer.push(limit + 1));
	EXPECT_TRUE(buffer.push(2 * limit));
}

int main()
{
	testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}