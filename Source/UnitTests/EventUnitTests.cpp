#include "Precomp.h"

#include "Core/UnitTests.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Components/EventTestingComponent.h"

UNIT_TEST(Events, OnTick)
{
	using namespace Engine;

	World world{ true };
	const entt::entity owner = world.GetRegistry().Create();

	const EventTestingComponent& eventTester = world.GetRegistry().AddComponent<EventTestingComponent>(owner);
	TEST_ASSERT(eventTester.mOwner == owner);

	// The expected number of events will have to be re-evaluated once more events have been implemented
	TEST_ASSERT(eventTester.mTotalNumOfEventsCalled == 0);
	TEST_ASSERT(eventTester.mNumOfTicks == 0);

	world.Tick(1 / 60.0f);

	// The expected number of events will have to be re-evaluated once more events have been implemented
	TEST_ASSERT(eventTester.mTotalNumOfEventsCalled == 1);
	TEST_ASSERT(eventTester.mNumOfTicks == 1);

	TEST_ASSERT(eventTester.mLastReceivedOwner == owner);
	TEST_ASSERT(eventTester.mLastReceivedWorld == &world);

	return UnitTest::Success;
}
