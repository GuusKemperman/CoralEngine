#include "Precomp.h"
#include "Utilities/AbilityFunctionality.h"

#include "Components/TransformComponent.h"
#include "Components/Abilities/ActiveAbilityComponent.h"
#include "Components/Abilities/AOEComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Abilities/EffectsOnCharacterComponent.h"
#include "Components/Abilities/ProjectileComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Utilities/Math.h"

Engine::MetaType Engine::AbilityFunctionality::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityFunctionality>{}, "AbilityFunctionality" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddFunc([](entt::entity castByEntity, entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			ApplyInstantEffect(*world, castByEntity, affectedEntity, EffectSettings{ stat, amount, flatOrPercentage, increaseOrDecrease });

		}, "ApplyInstantEffect", MetaFunc::ExplicitParams<
		entt::entity, entt::entity, Stat, float, FlatOrPercentage, IncreaseOrDecrease>{}, "CastByEntity", "ApplyToEntity", "Stat", "Amount").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](entt::entity castByEntity, entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease, float duration)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			ApplyDurationalEffect(*world, castByEntity, affectedEntity, EffectSettings{ stat, amount, flatOrPercentage, increaseOrDecrease }, duration);

		}, "ApplyDurationalEffect", MetaFunc::ExplicitParams<
		entt::entity, entt::entity, Stat, float, FlatOrPercentage, IncreaseOrDecrease, float>{}, "CastByEntity", "ApplyToEntity", "Stat", "Amount", "FlatOrPercentage", "IncreaseOrDecrease", "Duration").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](entt::entity castByEntity, entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease, float duration, int ticks)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			ApplyOverTimeEffect(*world, castByEntity, affectedEntity, EffectSettings{ stat, amount, flatOrPercentage, increaseOrDecrease }, duration, ticks);

		}, "ApplyOverTimeEffect", MetaFunc::ExplicitParams<
		entt::entity, entt::entity, Stat, float, FlatOrPercentage, IncreaseOrDecrease, float, int>{}, "CastByEntity", "ApplyToEntity", "Stat", "Amount", "FlatOrPercentage", "IncreaseOrDecrease", "TickDuration", "NumberOfTicks").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

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

std::optional<float> Engine::AbilityFunctionality::ApplyInstantEffect(World& world, entt::entity castByEntity, entt::entity affectedEntity, EffectSettings effect)
{
	auto& reg = world.GetRegistry();
	auto characterComponent = reg.TryGet<CharacterComponent>(affectedEntity);
	if (characterComponent == nullptr)
	{
		LOG(LogAbilitySystem, Error, "Apply Effect - AffectedEntity {} is not a character.", entt::to_integral(affectedEntity));
		return std::nullopt;
	}
	auto [base, current] = GetStat(effect.mStat, *characterComponent);

	if (effect.mFlatOrPercentage == FlatOrPercentage::Percentage)
	{
		effect.mAmount = effect.mAmount * 0.01f * base;
	}

	if (effect.mIncreaseOrDecrease == IncreaseOrDecrease::Decrease)
	{
		if (effect.mStat == Stat::Health)
		{
			auto castByCharacterComponent = reg.TryGet<CharacterComponent>(castByEntity);
			if (castByCharacterComponent == nullptr)
			{
				LOG(LogAbilitySystem, Error, "Apply Effect - CastByEntity {} is not a character.", entt::to_integral(castByEntity));
				return std::nullopt;
			}
			const float damageModifier =
				(castByCharacterComponent->mCurrentDealtDamageModifier + characterComponent->mCurrentReceivedDamageModifier)
				* 0.01f;
			effect.mAmount += effect.mAmount * damageModifier;
		}

		effect.mAmount = -effect.mAmount;
	}

	// apply
	current += effect.mAmount;
	//current = std::max(current, 0.0f);
	//if (ability.clampToMax)
		//current = std::min(current, base);

	return effect.mAmount;
}

void Engine::AbilityFunctionality::ApplyDurationalEffect(World& world, entt::entity castByEntity, entt::entity affectedEntity, EffectSettings effect, float duration)
{
	const auto calculatedAmount = ApplyInstantEffect(world, castByEntity, affectedEntity, effect);
	if (!calculatedAmount.has_value())
	{
		return;
	}
	auto& reg = world.GetRegistry();
	auto effects = reg.TryGet<EffectsOnCharacterComponent>(affectedEntity);
	if (effects == nullptr)
	{
		LOG(LogAbilitySystem, Error, "Apply Effect - AffectedEntity {} does not have EffectsOnCharacterComponent attached.", entt::to_integral(affectedEntity));
		return;
	}
	effects->mDurationalEffects.push_back(DurationalEffect{ duration, 0.f, effect.mStat, calculatedAmount.value() });
}

void Engine::AbilityFunctionality::RevertDurationalEffect(CharacterComponent& characterComponent, DurationalEffect& durationalEffect)
{
	float& currentStat = GetStat(durationalEffect.mStatAffected, characterComponent).second;
	currentStat -= durationalEffect.mAmount;
}

void Engine::AbilityFunctionality::ApplyOverTimeEffect(World& world, entt::entity castByEntity, entt::entity affectedEntity, EffectSettings effect, float duration, int ticks)
{
	auto& reg = world.GetRegistry();
	auto effects = reg.TryGet<EffectsOnCharacterComponent>(affectedEntity);
	if (effects == nullptr)
	{
		LOG(LogAbilitySystem, Error, "Apply Effect - AffectedEntity {} does not have EffectsOnCharacterComponent attached.", entt::to_integral(affectedEntity));
		return;
	}
	auto castByEntityCharacterComponent = reg.TryGet<CharacterComponent>(castByEntity);
	if (castByEntityCharacterComponent == nullptr)
	{
		LOG(LogAbilitySystem, Error, "Apply Effect - AffectedEntity {} is not a character.", entt::to_integral(castByEntity));
		return;
	}

	effects->mOverTimeEffects.push_back(OverTimeEffect{ duration, 0.f, ticks, 0, EffectSettings{effect.mStat, effect.mAmount, effect.mFlatOrPercentage, effect.mIncreaseOrDecrease}, castByEntityCharacterComponent->mCurrentDealtDamageModifier });
}

void Engine::AbilityFunctionality::ApplyInstantEffectForOverTimeEffect(World& world, entt::entity affectedEntity, EffectSettings effect, float dealtDamageModifierOfCastByCharacter)
{
	auto& reg = world.GetRegistry();
	auto characterComponent = reg.TryGet<CharacterComponent>(affectedEntity);
	if (characterComponent == nullptr)
	{
		LOG(LogAbilitySystem, Error, "Apply Effect - AffectedEntity {} is not a character.", entt::to_integral(affectedEntity));
		return;
	}
	auto [base, current] = GetStat(effect.mStat, *characterComponent);

	if (effect.mFlatOrPercentage == FlatOrPercentage::Percentage)
	{
		effect.mAmount = effect.mAmount * 0.01f * base;
	}

	if (effect.mIncreaseOrDecrease == IncreaseOrDecrease::Decrease)
	{
		if (effect.mStat == Stat::Health)
		{
			const float damageModifier =
				(dealtDamageModifierOfCastByCharacter + characterComponent->mCurrentReceivedDamageModifier)
				* 0.01f;
			effect.mAmount += effect.mAmount * damageModifier;
		}

		effect.mAmount = -effect.mAmount;
	}

	// apply
	current += effect.mAmount;
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
	auto prefabPhysicsBody = reg.TryGet<PhysicsBody2DComponent>(prefabEntity);
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
	switch (stat)
	{
	case Health:
		return { characterComponent.mBaseHealth, characterComponent.mCurrentHealth };
	case MovementSpeed:
		return { characterComponent.mBaseMovementSpeed, characterComponent.mCurrentMovementSpeed };
	case DealtDamageModifier:
		return { characterComponent.mBaseDealtDamageModifier, characterComponent.mCurrentDealtDamageModifier };
	case ReceivedDamageModifier:
		return { characterComponent.mBaseReceivedDamageModifier, characterComponent.mCurrentReceivedDamageModifier };
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

bool Engine::AbilityFunctionality::EffectSettings::operator==(const EffectSettings& other) const
{
	return mStat == other.mStat &&
		Math::AreFloatsEqual(mAmount, other.mAmount) &&
		mFlatOrPercentage == other.mFlatOrPercentage &&
		mIncreaseOrDecrease == other.mIncreaseOrDecrease;
}

bool Engine::AbilityFunctionality::EffectSettings::operator!=(const EffectSettings& other) const
{
	return mStat != other.mStat ||
		!Math::AreFloatsEqual(mAmount, other.mAmount) ||
		mFlatOrPercentage != other.mFlatOrPercentage ||
		mIncreaseOrDecrease != other.mIncreaseOrDecrease;
}

Engine::MetaType Engine::AbilityFunctionality::EffectSettings::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<EffectSettings>{}, "EffectSettings" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&EffectSettings::mStat, "mStat").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&EffectSettings::mAmount, "mAmount").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&EffectSettings::mFlatOrPercentage, "mFlatOrPercentage").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&EffectSettings::mIncreaseOrDecrease, "mIncreaseOrDecrease").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);

	return metaType;
}
