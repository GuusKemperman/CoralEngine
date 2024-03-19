#include "Precomp.h"
#include "Utilities/AbilityFunctionality.h"

#include "Components/TransformComponent.h"
#include "Components/Abilities/ActiveAbilityComponent.h"
#include "Components/Abilities/AOEComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Abilities/ProjectileComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "World/Registry.h"
#include "World/World.h"

Engine::MetaType Engine::AbilityFunctionality::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityFunctionality>{}, "AbilityFunctionality" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddFunc([](entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			ApplyInstantEffect(*world, affectedEntity, stat, amount, flatOrPercentage, increaseOrDecrease);

		}, "ApplyInstantEffect", MetaFunc::ExplicitParams<
		entt::entity, Stat, float, FlatOrPercentage, IncreaseOrDecrease>{}, "ApplyToEntity", "Stat", "Amount").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](const std::shared_ptr<const Prefab>& prefab, entt::entity castBy) -> entt::entity
		{
			if (prefab == nullptr)
			{
				LOG(LogWorld, Warning, "Attempted to spawn NULL prefab.");
				return entt::null;
			}

			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			return SpawnProjectile(*world, *prefab, castBy);

		}, "SpawnProjectile", MetaFunc::ExplicitParams<
		const std::shared_ptr<const Prefab>&, entt::entity>{}, "Prefab", "Cast By").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](const std::shared_ptr<const Prefab>& prefab, entt::entity castBy) -> entt::entity
		{
			if (prefab == nullptr)
			{
				LOG(LogWorld, Warning, "Attempted to spawn NULL prefab.");
				return entt::null;
			}

			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			return SpawnAOE(*world, *prefab, castBy);

		}, "SpawnAOE", MetaFunc::ExplicitParams<
		const std::shared_ptr<const Prefab>&, entt::entity>{}, "Prefab", "Cast By").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	return metaType;
}

void Engine::AbilityFunctionality::ApplyInstantEffect(World& world, entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease)
{
	auto& reg = world.GetRegistry();
	auto& characterComponent = reg.Get<CharacterComponent>(affectedEntity);
	auto [base, current] = GetStat(stat, characterComponent);

	if (flatOrPercentage == FlatOrPercentage::Percentage)
	{
		amount = amount * 0.01f * base;
	}

	// in the future apply damage modifier

	if (increaseOrDecrease == IncreaseOrDecrease::Decrease)
	{
		amount = -amount;
	}

	// apply
	current += amount;
	current = std::max(current, 0.0f);
	//if (ability.clampToMax)
		current = std::min(current, base);
}

entt::entity Engine::AbilityFunctionality::SpawnProjectile(World& world, const Prefab& prefab, entt::entity castBy)
{
	auto& reg = world.GetRegistry();
	auto prefabEntity = reg.CreateFromPrefab(prefab);

	auto prejectileComponent = reg.TryGet<ProjectileComponent>(prefabEntity);
	if (prejectileComponent == nullptr)
	{
		LOG(LogAbilitySystem, Error, "The prefab does not have a ProjectileComponent attached.")
			return{};
	}
	auto activeAbility = reg.TryGet<ActiveAbilityComponent>(prefabEntity);
	if (activeAbility == nullptr)
	{
		LOG(LogAbilitySystem, Error, "The prefab does not have an ActiveAbilityComponent attached.")
			return{};
	}
	auto prefabTransform = reg.TryGet<TransformComponent>(prefabEntity);
	if (prefabTransform == nullptr)
	{
		LOG(LogAbilitySystem, Error, "The prefab does not have a TransformComponent attached.")
			return{};
	}
	auto prefabPhysicsBody= reg.TryGet<PhysicsBody2DComponent>(prefabEntity);
	if (prefabPhysicsBody == nullptr)
	{
		LOG(LogAbilitySystem, Error, "The prefab does not have a PhysicsBody2DComponent attached.")
			return{};
	}
	auto characterTransform = reg.TryGet<TransformComponent>(castBy);
	if (characterTransform == nullptr)
	{
		LOG(LogAbilitySystem, Error, "The cast-by character does not have a TransformComponent attached.")
			return{};
	}

	activeAbility->mCastByCharacter = castBy;
	auto characterWorldPos = characterTransform->GetWorldPosition();
	prefabTransform->SetLocalPosition(characterWorldPos);
	auto characterDirection = Math::QuatToDirectionXZ(characterTransform->GetWorldOrientation());
	prefabPhysicsBody->mLinearVelocity = characterDirection * prejectileComponent->mSpeed;

	return prefabEntity;
}

entt::entity Engine::AbilityFunctionality::SpawnAOE(World& world, const Prefab& prefab, entt::entity castBy)
{
	auto& reg = world.GetRegistry();
	auto prefabEntity = reg.CreateFromPrefab(prefab);

	auto activeAbility = reg.TryGet<ActiveAbilityComponent>(prefabEntity);
	if (activeAbility == nullptr)
	{
		LOG(LogAbilitySystem, Error, "The prefab does not have an ActiveAbilityComponent attached.")
			return{};
	}
	auto prefabTransform = reg.TryGet<TransformComponent>(prefabEntity);
	if (prefabTransform == nullptr)
	{
		LOG(LogAbilitySystem, Error, "The prefab does not have a TransformComponent attached.")
			return{};
	}
	auto characterTransform = reg.TryGet<TransformComponent>(castBy);
	if (characterTransform == nullptr)
	{
		LOG(LogAbilitySystem, Error, "The cast-by character does not have a TransformComponent attached.")
			return{};
	}

	activeAbility->mCastByCharacter = castBy;
	auto characterWorldPos = characterTransform->GetWorldPosition();
	prefabTransform->SetLocalPosition(characterWorldPos);

	return prefabEntity;
}

std::pair<float&, float&> Engine::AbilityFunctionality::GetStat(Stat stat, CharacterComponent& characterComponent)
{
	switch(stat)
	{
	case Health:
		return { characterComponent.mBaseHealth, characterComponent.mCurrentHealth };
	case MovementSpeed:
		return { characterComponent.mBaseMovementSpeed, characterComponent.mCurrentMovementSpeed };
	}

	// because of the stupid warning "not all control paths return a value"
	return { characterComponent.mBaseHealth, characterComponent.mCurrentHealth };
}

Engine::MetaType Reflector<Engine::AbilityFunctionality::Stat>::Reflect()
{
	using namespace Engine;
	using T = AbilityFunctionality::Stat;
	MetaType type{ MetaType::T<T>{}, "Stat" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

Engine::MetaType Reflector<Engine::AbilityFunctionality::FlatOrPercentage>::Reflect()
{
	using namespace Engine;
	using T = AbilityFunctionality::FlatOrPercentage;
	MetaType type{ MetaType::T<T>{}, "FlatOrPercentage" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

Engine::MetaType Reflector<Engine::AbilityFunctionality::IncreaseOrDecrease>::Reflect()
{
	using namespace Engine;
	using T = AbilityFunctionality::IncreaseOrDecrease;
	MetaType type{ MetaType::T<T>{}, "IncreaseOrDecrease" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}
