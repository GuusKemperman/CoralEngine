#include "Precomp.h"

#include "Core/UnitTests.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Components/EventTestingComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Utilities/Events.h"
#include "Meta/MetaType.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"

namespace CE
{
	class EnemyAiControllerComponent;

	static const MetaType* GetUnitTestScript()
	{
		return MetaManager::Get().TryGetType("UnitTestScript");
	}

	template <bool TestScriptingSide, bool TestEmpty>
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
		return DoesValueMatch<false, false>(world.GetRegistry().TryGet(MakeTypeId<EventTestingComponent>(), entity),
		                                    nameOfField, expectedValue)
			&& DoesValueMatch<true, false>(world.GetRegistry().TryGet(GetUnitTestScript()->GetTypeId(), entity),
			                               nameOfField, expectedValue)
			&& DoesValueMatch<false, true>(MetaAny{MakeTypeInfo<EmptyEventTestingComponent>(), nullptr, false},
			                               nameOfField, expectedValue);
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
	using namespace CE;

	World world{true};

	entt::entity owner = InitTest(world);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfTicks", 0));

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfTicks", 1));

	return UnitTest::Success;
}

UNIT_TEST(Events, OnFixedTick)
{
	using namespace CE;

	World world{true};
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
	using namespace CE;

	World world{false};
	entt::entity owner = InitTest(world);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfConstructs", 1));

	return UnitTest::Success;
}

UNIT_TEST(Events, OnBeginPlay)
{
	using namespace CE;

	World world{false};
	entt::entity owner = InitTest(world);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfBeginPlays", 0));

	world.BeginPlay();

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfBeginPlays", 1));

	return UnitTest::Success;
}

UNIT_TEST(Events, OnBeginPlayWhenAddedAfterWorldBeginsPlay)
{
	using namespace CE;

	World world{true};
	entt::entity owner = InitTest(world);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfBeginPlays", 1));

	return UnitTest::Success;
}

UNIT_TEST(Events, OnDestructEntireWorld)
{
	using namespace CE;

	{
		World world{true};
		entt::entity owner = InitTest(world);

		TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfDestructs", 0));
	}

	// I guess we can't really test the num of destructs if the instance were destroyed..
	// But we have the static one atleast
	TEST_ASSERT(EmptyEventTestingComponent::sNumOfDestructs == 1);

	return UnitTest::Success;
}

UNIT_TEST(Events, OnDestructRemoveComponent)
{
	using namespace CE;

	World world{true};
	entt::entity owner = InitTest(world);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfDestructs", 0));

	world.GetRegistry().RemoveComponent<EmptyEventTestingComponent>(owner);

	// I guess we can't really test the num of destructs if the instance were destroyed..
	// But we have the static one atleast
	TEST_ASSERT(EmptyEventTestingComponent::sNumOfDestructs == 1);

	return UnitTest::Success;
}

UNIT_TEST(Events, OnDestructDestroyEntity)
{
	using namespace CE;

	World world{true};
	entt::entity owner = InitTest(world);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfDestructs", 0));

	world.GetRegistry().Destroy(owner, false);
	world.GetRegistry().RemovedDestroyed();

	// I guess we can't really test the num of destructs if the instance were destroyed..
	// But we have the static one atleast
	TEST_ASSERT(EmptyEventTestingComponent::sNumOfDestructs == 1);

	return UnitTest::Success;
}

UNIT_TEST(Events, OnAiTick)
{
	using namespace CE;

	World world{true};
	entt::entity owner = InitTest(world);

	world.GetRegistry().AddComponent<EnemyAiControllerComponent>(owner);

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(EmptyEventTestingComponent::sNumOfAiTicks == 1);

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(EmptyEventTestingComponent::sNumOfAiTicks == 2);

	return UnitTest::Success;
}

UNIT_TEST(Events, OnAiEvaluate)
{
	using namespace CE;

	World world{true};
	entt::entity owner = InitTest(world);

	world.GetRegistry().AddComponent<EnemyAiControllerComponent>(owner);

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(EmptyEventTestingComponent::sNumOfAiEvaluates == 1);

	world.Tick(sFixedTickEventStepSize * .5f);

	TEST_ASSERT(EmptyEventTestingComponent::sNumOfAiEvaluates == 2);

	return UnitTest::Success;
}

UNIT_TEST(Events, CollisionEvents)
{
	using namespace CE;

	World world{true};
	const entt::entity owner = InitTest(world);

	Registry& reg = world.GetRegistry();

	reg.AddComponent<TransformComponent>(owner);
	
	PhysicsBody2DComponent& body1 = reg.AddComponent<PhysicsBody2DComponent>(owner);
	body1.mIsAffectedByForces = false;
	body1.mRules = CollisionPresets::sCharacter.mRules;
	reg.AddComponent<DiskColliderComponent>(owner);

	const entt::entity other = reg.Create();

	TransformComponent& otherTransform = reg.AddComponent<TransformComponent>(other);
	PhysicsBody2DComponent& body2 = reg.AddComponent<PhysicsBody2DComponent>(other);
	body2.mIsAffectedByForces = false;
	body2.mRules = CollisionPresets::sCharacter.mRules;
	reg.AddComponent<DiskColliderComponent>(other);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionEntry", 0));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionStay", 0));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionExit", 0));

	world.Tick(1 / 60.0f - 0.001f);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionEntry", 1));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionStay", 1));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionExit", 0));

	world.Tick(1 / 60.0f - 0.001f);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionEntry", 1));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionStay", 2));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionExit", 0));

	world.Tick(1 / 60.0f - 0.001f);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionEntry", 1));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionStay", 3));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionExit", 0));

	otherTransform.SetWorldPosition(glm::vec2{100000.0f});

	world.Tick(1 / 60.0f - 0.001f);

	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionEntry", 1));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionStay", 3));
	TEST_ASSERT(DoBothValuesMatch(world, owner, "mNumOfCollisionExit", 1));

	return UnitTest::Success;
}
