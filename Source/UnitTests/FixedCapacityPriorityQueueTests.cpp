#include "Precomp.h"

#include <queue>

#include "Utilities/FixedCapacityPriorityQueue.h"

#include "Core/UnitTests.h"
#include "Utilities/Random.h"

namespace
{
	template<typename T = int, size_t S = 256>
	void TestBoth(const auto& func)
	{
		std::priority_queue<int> stdQueue{};
		CE::FixedCapacityPriorityQueue<int, 256> ceQueue{};

		func(stdQueue);
		func(ceQueue);
	}
}

UNIT_TEST(FixedCapacityPriorityQueue, PushPop)
{
	TestBoth([](auto& queue)
		{
			queue.push(1);
			queue.push(2);
			queue.push(3);

			TEST_EQUAL(queue.top(), 3);
			queue.pop();

			TEST_EQUAL(queue.top(), 2);
			queue.pop();

			TEST_EQUAL(queue.top(), 1);
			queue.pop();

			for (int i = 0; i < 256; i++)
			{
				queue.push(i);
				TEST_EQUAL(queue.top(), i);
			}

			while (!queue.empty())
			{
				queue.pop();
			}

			int highestNum = std::numeric_limits<int>::min();
			for (int i = 0; i < 256; i++)
			{
				int randomNum = CE::Random::Range(-1000, 1000);
				queue.push(randomNum);

				highestNum = std::max(highestNum, randomNum);
				TEST_EQUAL(queue.top(), highestNum);
			}
		});
}
