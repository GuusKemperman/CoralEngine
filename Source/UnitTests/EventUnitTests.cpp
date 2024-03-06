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

	template<bool TestScriptingSide, bool TestEmpty>
	static bool DoesValueMatch(MetaAny component, std::string_view nameOfField, uint32 expectedValue)
	{
		if constexpr (TestEmpty)
		{
			return EmptyEventTestingComponent::GetValue(nameOfField) == expectedValue;
		}
		else
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
	}

	static bool DoBothValuesMatch(World& world, entt::entity entity, std::string_view nameOfField, uint32 expectedValue)
	{
		return DoesValueMatch<false, false>(world.GetRegistry().TryGet(MakeTypeId<EventTestingComponent>(), entity), nameOfField, expectedValue)
			&& DoesValueMatch<true, false>(world.GetRegistry().TryGet(GetUnitTestScript()->GetTypeId(), entity), nameOfField, expectedValue)
			&& DoesValueMatch<false, true>(MetaAny{ MakeTypeInfo<EmptyEventTestingComponent>(), nullptr, false }, nameOfField, expectedValue);
	}

	static entt::entity InitTest(World& world)
	{
		EmptyEventTestingComponent::Reset();
		const entt::entity owner = world.GetRegistry().Create();
		world.GetRegistry().AddComponent<EventTestingComponent>(owner);
		world.GetRegistry().AddComponent<EmptyEventTestingComponent>(owner);

		const MetaType* const unitTestScript = GetUnitTestScript();
		if (unitTestScript == nullptr)
		{
			LOG(LogUnitTests, Error, "No unit test script");
			return entt::null;
		}
		world.GetRegistry().AddComponent(*unitTestScript, owner);

		return owner;
	}
}

UNIT_TEST(Events, OnTick)
{
	using namespace Engine;

	World world{ true };

	entt::entity owner = InitTest(world);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfTicks", 0));

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfTicks", 1));

	return UnitTest::Success;
}

UNIT_TEST(Events, OnFixedTick)
{
	using namespace Engine;

	World world{ true };
	entt::entity owner = InitTest(world);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfFixedTicks", 0));

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfFixedTicks", 1));

	world.Tick(sFixedTickEventStepSize * .6f);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfFixedTicks", 2));

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfFixedTicks", 2));

	return UnitTest::Success;
}

UNIT_TEST(Events, OnConstruct)
{
	using namespace Engine;

	World world{ false };
	entt::entity owner = InitTest(world);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfConstructs", 1));

	return UnitTest::Success;
}

UNIT_TEST(Events, OnBeginPlay)
{
	using namespace Engine;

	World world{ false };
	entt::entity owner = InitTest(world);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfBeginPlays", 0));

	world.BeginPlay();

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfBeginPlays", 1));

	return UnitTest::Success;
}

UNIT_TEST(Events, OnBeginPlayWhenAddedAfterWorldBeginsPlay)
{
	using namespace Engine;

	World world{ true };
	entt::entity owner = InitTest(world);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfBeginPlays", 1));

	return UnitTest::Success;
}