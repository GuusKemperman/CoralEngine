#include "Precomp.h"

#include "Core/UnitTests.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Components/EventTestingComponent.h"
#include "Utilities/Events.h"

UNIT_TEST(Events, OnTick)
{
	using namespace Engine;

	World world{ true };
	const entt::entity owner = world.GetRegistry().Create();

	const EventTestingComponent& eventTester = world.GetRegistry().AddComponent<EventTestingComponent>(owner);
	TEST_ASSERT(eventTester.mOwner == owner);

	TEST_ASSERT(eventTester.mTotalNumOfEventsCalled == 0);
	TEST_ASSERT(eventTester.mNumOfTicks == 0);

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(eventTester.mTotalNumOfEventsCalled == 2);
	TEST_ASSERT(eventTester.mNumOfTicks == 1);

	TEST_ASSERT(eventTester.mLastReceivedOwner == owner);
	TEST_ASSERT(eventTester.mLastReceivedWorld == &world);

	return UnitTest::Success;
}

UNIT_TEST(Events, OnFixedTick)
{
	using namespace Engine;

	World world{ true };
	const entt::entity owner = world.GetRegistry().Create();

	const EventTestingComponent& eventTester = world.GetRegistry().AddComponent<EventTestingComponent>(owner);
	TEST_ASSERT(eventTester.mOwner == owner);

	TEST_ASSERT(eventTester.mTotalNumOfEventsCalled == 0);
	TEST_ASSERT(eventTester.mNumOfFixedTicks == 0);

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(eventTester.mTotalNumOfEventsCalled == 2);
	TEST_ASSERT(eventTester.mNumOfFixedTicks == 1);

	world.Tick(sFixedTickEventStepSize * .6f);

	TEST_ASSERT(eventTester.mTotalNumOfEventsCalled == 4);
	TEST_ASSERT(eventTester.mNumOfFixedTicks == 2);

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(eventTester.mNumOfFixedTicks == 2);
	TEST_ASSERT(eventTester.mTotalNumOfEventsCalled == 5);

	TEST_ASSERT(eventTester.mLastReceivedOwner == owner);
	TEST_ASSERT(eventTester.mLastReceivedWorld == &world);

	return UnitTest::Success;
}
