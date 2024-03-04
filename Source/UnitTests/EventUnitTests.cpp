#include "Precomp.h"

#include "Core/UnitTests.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Components/EventTestingComponent.h"
#include "Utilities/Events.h"

namespace Engine
{
	static const MetaType* GetUnitTestScript()
	{
		return MetaManager::Get().TryGetType("UnitTestScript");
	}

	template<bool TestScriptingSide>
	static bool DoesValueMatch(MetaAny component, std::string_view nameOfField, uint32 expectedValue)
	{
		const MetaType* type{};

		if constexpr (TestScriptingSide)
		{
			type = GetUnitTestScript();

			if (type == nullptr)
			{
				LOG(LogUnitTests, Error, "UnitTestScript is missing");
				return false;
			}
		}
		else
		{
			type = &MetaManager::Get().GetType<EventTestingComponent>();
		}

		const MetaField* field = type->TryGetField(nameOfField);

		if (field == nullptr)
		{
			LOG(LogUnitTests, Error, "{} is missing", nameOfField);
			return false;
		}

		if (field->GetType().GetTypeId() != MakeTypeId<uint32>())
		{
			LOG(LogUnitTests, Error, "{} is not an uint32", nameOfField);
			return false;
		}

		MetaAny value = field->MakeRef(component);
		const uint32 actualValue = *value.As<uint32>();

		return actualValue == expectedValue;
	}

	static bool DoBothValuesMatch(World& world, entt::entity entity, std::string_view nameOfField, uint32 expectedValue)
	{
		return DoesValueMatch<false>(world.GetRegistry().TryGet(MakeTypeId<EventTestingComponent>(), entity), nameOfField, expectedValue)
			&& DoesValueMatch<true>(world.GetRegistry().TryGet(GetUnitTestScript()->GetTypeId(), entity), nameOfField, expectedValue);
	}
}

UNIT_TEST(Events, OnTick)
{
	using namespace Engine;

	World world{ true };
	const entt::entity owner = world.GetRegistry().Create();

	const EventTestingComponent& eventTester = world.GetRegistry().AddComponent<EventTestingComponent>(owner);
	TEST_ASSERT(eventTester.mOwner == owner);

	const MetaType* const unitTestScript = GetUnitTestScript();
	TEST_ASSERT(unitTestScript != nullptr);
	world.GetRegistry().AddComponent(*unitTestScript, owner);


	TEST_ASSERT(DoBothValuesMatch(world, owner, "mTotalNumOfEventsCalled", 0));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfTicks", 0));

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mTotalNumOfEventsCalled", 2));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfTicks", 1));

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

	const MetaType* const unitTestScript = GetUnitTestScript();
	TEST_ASSERT(unitTestScript != nullptr);
	world.GetRegistry().AddComponent(*unitTestScript, owner);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mTotalNumOfEventsCalled", 0));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfFixedTicks", 0));

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mTotalNumOfEventsCalled", 2));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfFixedTicks", 1));

	world.Tick(sFixedTickEventStepSize * .6f);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mTotalNumOfEventsCalled", 4));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfFixedTicks", 2));

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mTotalNumOfEventsCalled", 5));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfFixedTicks", 2));

	TEST_ASSERT(eventTester.mLastReceivedOwner == owner);
	TEST_ASSERT(eventTester.mLastReceivedWorld == &world);

	return UnitTest::Success;
}
