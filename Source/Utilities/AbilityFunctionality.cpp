#include "Precomp.h"
#include "Utilities/AbilityFunctionality.h"

#include "Components/TransformComponent.h"
#include "Components/Abilities/ActiveAbilityComponent.h"
#include "Components/Abilities/AbilityLifetimeComponent.h"
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
#include "Assets/Prefabs/Prefab.h"

CE::MetaType CE::AbilityFunctionality::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityFunctionality>{}, "AbilityFunctionality" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddFunc([](const CharacterComponent& castByCharacterData, entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease, bool clampToMax)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			ApplyInstantEffect(*world, castByCharacterData, affectedEntity, AbilityEffect{ stat, amount, flatOrPercentage, increaseOrDecrease, clampToMax });

		}, "ApplyInstantEffect", MetaFunc::ExplicitParams<
		const CharacterComponent&, entt::entity, Stat, float, FlatOrPercentage, IncreaseOrDecrease, bool>{}, "CastByCharacterData", "ApplyToEntity", "Stat", "Amount", "FlatOrPercentage", "IncreaseOrDecrease", "ClampToMax").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

		metaType.AddFunc([](const CharacterComponent& castByCharacterData, entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease, bool clampToMax, float duration)
			{
				World* world = World::TryGetWorldAtTopOfStack();
				ASSERT(world != nullptr);

				ApplyDurationalEffect(*world, castByCharacterData, affectedEntity, AbilityEffect{ stat, amount, flatOrPercentage, increaseOrDecrease, clampToMax }, duration);

			}, "ApplyDurationalEffect", MetaFunc::ExplicitParams<
			const CharacterComponent&, entt::entity, Stat, float, FlatOrPercentage, IncreaseOrDecrease, bool, float>{}, "CastByCharacterData", "ApplyToEntity", "Stat", "Amount", "FlatOrPercentage", "IncreaseOrDecrease", "ClampToMax", "Duration").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

			metaType.AddFunc([](const CharacterComponent& castByCharacterData, entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease, bool clampToMax, float duration, int ticks)
				{
					World* world = World::TryGetWorldAtTopOfStack();
					ASSERT(world != nullptr);

					ApplyOverTimeEffect(*world, castByCharacterData, affectedEntity, AbilityEffect{ stat, amount, flatOrPercentage, increaseOrDecrease, clampToMax }, duration, ticks);

				}, "ApplyOverTimeEffect", MetaFunc::ExplicitParams<
				const CharacterComponent&, entt::entity, Stat, float, FlatOrPercentage, IncreaseOrDecrease, bool, float, int>{}, "CastByCharacterData", "ApplyToEntity", "Stat", "Amount", "FlatOrPercentage", "IncreaseOrDecrease", "ClampToMax", "TickDuration", "NumberOfTicks").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

				metaType.AddFunc([](const AssetHandle<Prefab>& prefab, entt::entity castBy) -> entt::entity
					{
						if (prefab == nullptr)
						{
							LOG(LogWorld, Warning, "Attempted to spawn NULL prefab.");
							return entt::null;
						}

						World* world = World::TryGetWorldAtTopOfStack();
						ASSERT(world != nullptr);

						return SpawnAbilityPrefab(*world, *prefab, castBy);

					}, "SpawnAbilityPrefab", MetaFunc::ExplicitParams<
					const AssetHandle<Prefab>&, entt::entity>{}, "Prefab", "Cast By").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

					return metaType;
}

std::optional<float> CE::AbilityFunctionality::ApplyInstantEffect(World& world, const CharacterComponent& castByCharacterData, entt::entity affectedEntity, AbilityEffect effect)
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
			const float damageModifier = characterComponent->mCurrentReceivedDamageModifier + castByCharacterData.mCurrentDealtDamageModifier;
			effect.mAmount += effect.mAmount * damageModifier * 0.01f;
		}

		effect.mAmount = -effect.mAmount;
	}

	// Apply
	current += effect.mAmount;
	if (effect.mClampToMax)
	{
		current = std::min(current, base);
	}

	// Visual effect
	auto effects = reg.TryGet<EffectsOnCharacterComponent>(affectedEntity);
	if (effects == nullptr)
	{
		LOG(LogAbilitySystem, Error, "Apply Effect - AffectedEntity {} does not have EffectsOnCharacterComponent attached.", entt::to_integral(affectedEntity));
		return std::nullopt;
	}
	effects->mVisualEffects.push_back(VisualEffect{ GetEffectColor(effect.mStat, effect.mIncreaseOrDecrease) });

	return effect.mAmount;
}

void CE::AbilityFunctionality::ApplyDurationalEffect(World& world, const CharacterComponent& castByCharacterData, entt::entity affectedEntity, AbilityEffect effect, float duration)
{
	const auto calculatedAmount = ApplyInstantEffect(world, castByCharacterData, affectedEntity, effect);
	if (!calculatedAmount.has_value())
	{
		return;
	}

	auto& reg = world.GetRegistry();
	// We do a Get() here because we have already checked for this component in ApplyInstantEffect()
	auto& effects = reg.Get<EffectsOnCharacterComponent>(affectedEntity);

	effects.mDurationalEffects.push_back(DurationalEffect{ duration, 0.f, effect.mStat, calculatedAmount.value() });
	effects.mVisualEffects.back().mDuration = duration;
}

void CE::AbilityFunctionality::RevertDurationalEffect(CharacterComponent& characterComponent, const DurationalEffect& durationalEffect)
{
	float& currentStat = GetStat(durationalEffect.mStatAffected, characterComponent).second;
	currentStat -= durationalEffect.mAmount;
}

void CE::AbilityFunctionality::ApplyOverTimeEffect(World& world, const CharacterComponent&, entt::entity affectedEntity, AbilityEffect effect, float duration, int ticks)
{
	auto& reg = world.GetRegistry();
	auto effects = reg.TryGet<EffectsOnCharacterComponent>(affectedEntity);
	if (effects == nullptr)
	{
		LOG(LogAbilitySystem, Error, "Apply Effect - AffectedEntity {} does not have EffectsOnCharacterComponent attached.", entt::to_integral(affectedEntity));
		return;
	}

	effects->mOverTimeEffects.push_back(OverTimeEffect{ duration, 0.f, ticks, 0, AbilityEffect{effect.mStat, effect.mAmount, effect.mFlatOrPercentage, effect.mIncreaseOrDecrease} });
}

entt::entity CE::AbilityFunctionality::SpawnAbilityPrefab(World& world, const Prefab& prefab, entt::entity castBy)
{
	auto& reg = world.GetRegistry();
	auto prefabEntity = reg.CreateFromPrefab(prefab);

	auto activeAbility = reg.TryGet<ActiveAbilityComponent>(prefabEntity);
	if (activeAbility == nullptr)
	{
		LOG(LogAbilitySystem, Error, "The prefab does not have an ActiveAbilityComponent attached.");
		return{};
	}
	auto prefabTransform = reg.TryGet<TransformComponent>(prefabEntity);
	if (prefabTransform == nullptr)
	{
		LOG(LogAbilitySystem, Error, "The prefab does not have a TransformComponent attached.");
		return{};
	}
	auto characterTransform = reg.TryGet<TransformComponent>(castBy);
	if (characterTransform == nullptr)
	{
		LOG(LogAbilitySystem, Error, "The cast-by entity does not have a TransformComponent attached.");
		return{};
	}
	auto characterComponent = reg.TryGet<CharacterComponent>(castBy);
	if (characterComponent == nullptr)
	{
		LOG(LogAbilitySystem, Error, "The cast-by entity does not have a CharacterComponent attached.");
		return{};
	}

	// Store a copy of the cast-by character's CharacterComponent
	// so that effect calculations and team checks can be performed even if the character dies in the meantime.
	activeAbility->mCastByCharacterData = *characterComponent;

	// Set the position.
	const glm::vec3 pos = characterTransform->GetWorldPosition();
	prefabTransform->SetLocalPosition(pos);

	// Check for projectile component.
	auto projectileComponent = reg.TryGet<ProjectileComponent>(prefabEntity);
	if (projectileComponent != nullptr)
	{
		auto prefabPhysicsBody = reg.TryGet<PhysicsBody2DComponent>(prefabEntity);
		if (prefabPhysicsBody == nullptr)
		{
			LOG(LogAbilitySystem, Error, "The prefab does not have a PhysicsBody2DComponent attached.");
			return{};
		}
		// Calculate the 2D orientation of the character.
		const glm::vec2 characterDir = Math::QuatToDirectionXZ(characterTransform->GetWorldOrientation());
		// Set the velocity.
		prefabPhysicsBody->mLinearVelocity = characterDir * projectileComponent->mSpeed;

		// Translate the spawn position by a certain amount
		// so that the projectile does not spawn inside the character mesh.
		//const glm::vec2 projectileTranslation = characterDir * (characterTransform->GetWorldScale2D() + 1.f); // + 1.f arbitrary value
		//const glm::vec3 projectileTranslation3D = { projectileTranslation.x, 0.f, projectileTranslation.y };
		//const glm::vec3 projectileSpawnPos = characterTransform->GetWorldPosition() + projectileTranslation3D;
		//prefabTransform->SetLocalPosition(projectileSpawnPos);
		// I will leave commented code here for future use
	}

	return prefabEntity;
}

std::pair<float&, float&> CE::AbilityFunctionality::GetStat(Stat stat, CharacterComponent& characterComponent)
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

	// Needed because of the stupid warning "not all control paths return a value".
	return { characterComponent.mBaseHealth, characterComponent.mCurrentHealth };
}

glm::vec3 CE::AbilityFunctionality::GetEffectColor(Stat stat, IncreaseOrDecrease increaseOrDecrease)
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

CE::MetaType Reflector<CE::AbilityFunctionality::Stat>::Reflect()
{
	using namespace CE;
	using T = AbilityFunctionality::Stat;
	MetaType type{ MetaType::T<T>{}, "Stat" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

CE::MetaType Reflector<CE::AbilityFunctionality::FlatOrPercentage>::Reflect()
{
	using namespace CE;
	using T = AbilityFunctionality::FlatOrPercentage;
	MetaType type{ MetaType::T<T>{}, "FlatOrPercentage" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

CE::MetaType Reflector<CE::AbilityFunctionality::IncreaseOrDecrease>::Reflect()
{
	using namespace CE;
	using T = AbilityFunctionality::IncreaseOrDecrease;
	MetaType type{ MetaType::T<T>{}, "IncreaseOrDecrease" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

bool CE::AbilityFunctionality::AbilityEffect::operator==(const AbilityEffect& other) const
{
	return mStat == other.mStat &&
		Math::AreFloatsEqual(mAmount, other.mAmount) &&
		mFlatOrPercentage == other.mFlatOrPercentage &&
		mIncreaseOrDecrease == other.mIncreaseOrDecrease &&
		mClampToMax == other.mClampToMax;
}

bool CE::AbilityFunctionality::AbilityEffect::operator!=(const AbilityEffect& other) const
{
	return mStat != other.mStat ||
		!Math::AreFloatsEqual(mAmount, other.mAmount) ||
		mFlatOrPercentage != other.mFlatOrPercentage ||
		mIncreaseOrDecrease != other.mIncreaseOrDecrease ||
		mClampToMax != other.mClampToMax;
}

CE::MetaType CE::AbilityFunctionality::AbilityEffect::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityEffect>{}, "AbilityEffect" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&AbilityEffect::mStat, "mStat").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&AbilityEffect::mAmount, "mAmount").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&AbilityEffect::mFlatOrPercentage, "mFlatOrPercentage").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&AbilityEffect::mIncreaseOrDecrease, "mIncreaseOrDecrease").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&AbilityEffect::mClampToMax, "mClampToMax").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);

	return metaType;
}
