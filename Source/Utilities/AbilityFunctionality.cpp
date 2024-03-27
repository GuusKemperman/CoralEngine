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

	metaType.AddFunc([](entt::entity castByEntity, entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease, bool clampToMax)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			ApplyInstantEffect(*world, castByEntity, affectedEntity, EffectSettings{ stat, amount, flatOrPercentage, increaseOrDecrease, clampToMax });

		}, "ApplyInstantEffect", MetaFunc::ExplicitParams<
		entt::entity, entt::entity, Stat, float, FlatOrPercentage, IncreaseOrDecrease, bool>{}, "CastByEntity", "ApplyToEntity", "Stat", "Amount", "FlatOrPercentage", "IncreaseOrDecrease", "ClampToMax").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](entt::entity castByEntity, entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease, bool clampToMax, float duration)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			ApplyDurationalEffect(*world, castByEntity, affectedEntity, EffectSettings{ stat, amount, flatOrPercentage, increaseOrDecrease, clampToMax }, duration);

		}, "ApplyDurationalEffect", MetaFunc::ExplicitParams<
		entt::entity, entt::entity, Stat, float, FlatOrPercentage, IncreaseOrDecrease, bool, float>{}, "CastByEntity", "ApplyToEntity", "Stat", "Amount", "FlatOrPercentage", "IncreaseOrDecrease", "ClampToMax", "Duration").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](entt::entity castByEntity, entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease, bool clampToMax, float duration, int ticks)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			ApplyOverTimeEffect(*world, castByEntity, affectedEntity, EffectSettings{ stat, amount, flatOrPercentage, increaseOrDecrease, clampToMax }, duration, ticks);

		}, "ApplyOverTimeEffect", MetaFunc::ExplicitParams<
		entt::entity, entt::entity, Stat, float, FlatOrPercentage, IncreaseOrDecrease, bool, float, int>{}, "CastByEntity", "ApplyToEntity", "Stat", "Amount", "FlatOrPercentage", "IncreaseOrDecrease", "ClampToMax", "TickDuration", "NumberOfTicks").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

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

std::optional<float> Engine::AbilityFunctionality::ApplyInstantEffect(World& world, entt::entity castByEntity, entt::entity affectedEntity, EffectSettings effect, ApplyType applyType, float dealtDamageModifierOfCastByCharacter)
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
			float damageModifier = characterComponent->mCurrentReceivedDamageModifier;
			if (applyType != ApplyType::EffectOverTime)
			{
				auto castByCharacterComponent = reg.TryGet<CharacterComponent>(castByEntity);
				if (castByCharacterComponent == nullptr)
				{
					LOG(LogAbilitySystem, Error, "Apply Effect - CastByEntity {} is not a character.", entt::to_integral(castByEntity));
					return std::nullopt;
				}
				damageModifier += castByCharacterComponent->mCurrentDealtDamageModifier;
			}
			else
			{
				damageModifier += dealtDamageModifierOfCastByCharacter;
			}
			effect.mAmount += effect.mAmount * damageModifier * 0.01f;
		}

		effect.mAmount = -effect.mAmount;
	}

	// apply
	current += effect.mAmount;
	if (effect.mClampToMax)
	{
		current = std::min(current, base);
	}

	// visual effect
	auto effects = reg.TryGet<EffectsOnCharacterComponent>(affectedEntity);
	if (effects == nullptr)
	{
		LOG(LogAbilitySystem, Error, "Apply Effect - AffectedEntity {} does not have EffectsOnCharacterComponent attached.", entt::to_integral(affectedEntity));
		return std::nullopt;
	}
	effects->mVisualEffects.push_back(VisualEffect{ GetEffectColor(effect.mStat, effect.mIncreaseOrDecrease)});

	return effect.mAmount;
}

void Engine::AbilityFunctionality::ApplyDurationalEffect(World& world, entt::entity castByEntity, entt::entity affectedEntity, EffectSettings effect, float duration)
{
	const auto calculatedAmount = ApplyInstantEffect(world, castByEntity, affectedEntity, effect, ApplyType::Durational);
	if (!calculatedAmount.has_value())
	{
		return;
	}

	auto& reg = world.GetRegistry();
	// we do a Get() here because we have already checked for this component in ApplyInstantEffect()
	auto& effects = reg.Get<EffectsOnCharacterComponent>(affectedEntity);

	effects.mDurationalEffects.push_back(DurationalEffect{ duration, 0.f, effect.mStat, calculatedAmount.value() });
	effects.mVisualEffects.back().mDuration = duration;
}

void Engine::AbilityFunctionality::RevertDurationalEffect(CharacterComponent& characterComponent, const DurationalEffect& durationalEffect)
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

	// set the cast-by character for effect calculations and team checks
	activeAbility->mCastByCharacter = castBy;

	// calculate the 2D orientation of the character
	const glm::vec3 characterWorldPos = characterTransform->GetWorldPosition();
	const glm::vec2 characterDir = Math::QuatToDirectionXZ(characterTransform->GetWorldOrientation());

	// translate the spawn position by a certain amount so that the projectile does not spawn inside the character mesh
	const glm::vec2 projectileTranslation = characterDir * (characterTransform->GetWorldScale2D() + 1.f); // + 1.f arbitrary value
	const glm::vec3 projectileTranslation3D = { projectileTranslation.x, 0.f, projectileTranslation.y };
	const glm::vec3 projectileSpawnPos = characterWorldPos + projectileTranslation3D;

	// set the position
	prefabTransform->SetLocalPosition(projectileSpawnPos);

	// set the velocity
	prefabPhysicsBody->mLinearVelocity = characterDir * prejectileComponent->mSpeed;

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

	// set the cast-by character for effect calculations and team checks
	activeAbility->mCastByCharacter = castBy;

	// set the position
	const Physics& physics = world.GetPhysics();
	const glm::vec2 pos2D = characterTransform->GetWorldPosition2D();
	prefabTransform->SetLocalPosition(To3DRightForward(pos2D, physics.GetHeightAtPosition(pos2D)));

	return prefabEntity;
}

std::pair<float&, float&> Engine::AbilityFunctionality::GetStat(Stat stat, CharacterComponent& characterComponent)
{
	switch (stat)
	{
	case Stat::Health:
		return { characterComponent.mBaseHealth, characterComponent.mCurrentHealth };
	case Stat::MovementSpeed:
		return { characterComponent.mBaseMovementSpeed, characterComponent.mCurrentMovementSpeed };
	case Stat::DealtDamageModifier:
		return { characterComponent.mBaseDealtDamageModifier, characterComponent.mCurrentDealtDamageModifier };
	case Stat::ReceivedDamageModifier:
		return { characterComponent.mBaseReceivedDamageModifier, characterComponent.mCurrentReceivedDamageModifier };
	}

	// because of the stupid warning "not all control paths return a value"
	return { characterComponent.mBaseHealth, characterComponent.mCurrentHealth };
}

glm::vec3 Engine::AbilityFunctionality::GetEffectColor(Stat stat, IncreaseOrDecrease increaseOrDecrease)
{
	glm::vec3 color;
	if (const auto it = sDefaultEffectColors.find(std::make_pair(stat, increaseOrDecrease));
		it != sDefaultEffectColors.end())
	{
		color = it->second;
	}
	else
	{
		color = sDefaultEffectColors[std::make_pair(std::nullopt, increaseOrDecrease)];
	}
	return color;
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
		mIncreaseOrDecrease == other.mIncreaseOrDecrease && 
		mClampToMax == other.mClampToMax;
}

bool Engine::AbilityFunctionality::EffectSettings::operator!=(const EffectSettings& other) const
{
	return mStat != other.mStat ||
		!Math::AreFloatsEqual(mAmount, other.mAmount) ||
		mFlatOrPercentage != other.mFlatOrPercentage ||
		mIncreaseOrDecrease != other.mIncreaseOrDecrease ||
		mClampToMax != other.mClampToMax;
}

Engine::MetaType Engine::AbilityFunctionality::EffectSettings::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<EffectSettings>{}, "EffectSettings" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&EffectSettings::mStat, "mStat").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&EffectSettings::mAmount, "mAmount").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&EffectSettings::mFlatOrPercentage, "mFlatOrPercentage").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&EffectSettings::mIncreaseOrDecrease, "mIncreaseOrDecrease").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&EffectSettings::mClampToMax, "mClampToMax").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);

	return metaType;
}
