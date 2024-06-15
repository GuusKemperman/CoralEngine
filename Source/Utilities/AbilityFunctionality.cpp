#include "Precomp.h"
#include "Utilities/AbilityFunctionality.h"

#include "Assets/Ability/Weapon.h"
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
#include "Components/PlayerComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Systems/AbilitySystem.h"
#include "Utilities/Random.h"

CE::MetaType CE::AbilityFunctionality::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityFunctionality>{}, "AbilityFunctionality" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddFunc([](const CharacterComponent* castByCharacterData, entt::entity affectedEntity, AbilityEffect abilityEffect, bool doNotApplyColor) -> bool
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			return ApplyInstantEffect(*world, castByCharacterData, affectedEntity, abilityEffect, doNotApplyColor).second;

		}, "ApplyInstantEffect", MetaFunc::ExplicitParams<
		const CharacterComponent*, entt::entity, AbilityEffect, bool>{}, "CastByCharacterData", "ApplyToEntity", "AbilityEffect", "DoNotApplyColor", "CriticalHit").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](const CharacterComponent* castByCharacterData, entt::entity affectedEntity, AbilityEffect abilityEffect, float duration, bool doNotApplyColor)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			ApplyDurationalEffect(*world, castByCharacterData, affectedEntity, abilityEffect, duration, doNotApplyColor);

		}, "ApplyDurationalEffect", MetaFunc::ExplicitParams<
		const CharacterComponent*, entt::entity, AbilityEffect, float, bool>{}, "CastByCharacterData", "ApplyToEntity", "AbilityEffect", "Duration", "DoNotApplyColor").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](const CharacterComponent* castByCharacterData, entt::entity affectedEntity, AbilityEffect abilityEffect, float duration, int ticks)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			ApplyOverTimeEffect(*world, castByCharacterData, affectedEntity, abilityEffect, duration, ticks);

		}, "ApplyOverTimeEffect", MetaFunc::ExplicitParams<
		const CharacterComponent*, entt::entity, AbilityEffect, float, int>{}, "CastByCharacterData", "ApplyToEntity", "AbilityEffect", "TickDuration", "NumberOfTicks").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

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

	metaType.AddFunc([](const AssetHandle<Prefab>& prefab, entt::entity castBy, const AssetHandle<Weapon>& weapon) -> std::vector<entt::entity>
		{
			if (prefab == nullptr)
			{
				LOG(LogWorld, Warning, "SpawnProjectilePrefabs - Attempted to spawn NULL prefab.");
				return {};
			}
			if (weapon == nullptr)
			{
				LOG(LogWorld, Warning, "SpawnProjectilePrefabs - Weapon is NULL.");
				return {};
			}
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			return SpawnProjectilePrefabs(*world, *prefab, castBy, weapon);

		}, "SpawnProjectilePrefabs", MetaFunc::ExplicitParams<
		const AssetHandle<Prefab>&, entt::entity, const AssetHandle<Weapon>&>{}, "Prefab", "Cast By", "Weapon").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](const AssetHandle<Prefab>& prefab, entt::entity castBy, const WeaponInstance& weapon) -> std::vector<entt::entity>
		{
			if (prefab == nullptr)
			{
				LOG(LogWorld, Warning, "SpawnProjectilePrefabsFromWeaponInstance - Attempted to spawn NULL prefab.");
				return {};
			}
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			return SpawnProjectilePrefabsFromWeaponInstance(*world, *prefab, castBy, weapon);

		}, "SpawnProjectilePrefabsFromWeaponInstance", MetaFunc::ExplicitParams<
		const AssetHandle<Prefab>&, entt::entity, const WeaponInstance&>{}, "Prefab", "Cast By", "Weapon").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc(&AbilityFunctionality::IncreasePierceCountAndReturnTrueIfExceeded, "IncreasePierceCountAndReturnTrueIfExceeded").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddFunc([](entt::entity entityToAffect, entt::entity abilityEntity) -> bool
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			return WasTheAbilityCastByAnEnemy(*world, entityToAffect, abilityEntity);

		}, "WasTheAbilityCastByAnEnemy", MetaFunc::ExplicitParams<
		entt::entity, entt::entity>{}, "Entity To Affect", "Ability Entity").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	metaType.AddFunc([](float value1, float value2, float percentage) -> float
		{

			return IncreaseValue1ByPercentageOfValue2(value1, value2, percentage);

		}, "IncreaseValue1ByPercentageOfValue2", MetaFunc::ExplicitParams<
		float, float, float>{}, "Value 1", "Value 2", "Percentage").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	metaType.AddFunc([](const glm::vec2 point, const glm::vec2 coneOrigin, const glm::vec2 coneDirection, const float coneAngle) -> bool
			{

				return IsPointInsideCone2D(point, coneOrigin, coneDirection, coneAngle);

			}, "IsPointInsideCone2D", MetaFunc::ExplicitParams<
			const glm::vec2, const glm::vec2, const glm::vec2, const float>{}, "Point To Check", "Cone Origin", "Cone Direction", "Cone Angle").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	metaType.AddFunc([](entt::entity entity, WeaponInstance weapon)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			AddWeaponToEnd(*world, entity, weapon);

		}, "AddWeaponToEnd", MetaFunc::ExplicitParams<
		entt::entity, WeaponInstance>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](entt::entity entity, int index)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			RemoveWeaponAtIndex(*world, entity, index);

		}, "RemoveWeaponAtIndex", MetaFunc::ExplicitParams<
		entt::entity, int>{}, "Entity", "Index").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](entt::entity entity, AssetHandle<Weapon> weaponAsset)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			ReplaceWeaponAtEnd(*world, entity, weaponAsset);

		}, "ReplaceWeaponAtEnd", MetaFunc::ExplicitParams<
		entt::entity, AssetHandle<Weapon>>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](entt::entity characterEntity, entt::entity hitEntity, entt::entity abilityEntity)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			CallAllAbilityHitOrCritEvents(*world, characterEntity, hitEntity, abilityEntity, AbilitySystem::GetAbilityHitEvents());

		}, "CallAllAbilityHitEvents", MetaFunc::ExplicitParams<
		entt::entity, entt::entity, entt::entity>{}, "CharacterEntity", "HitEntity", "AbilityEntity").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](entt::entity characterEntity, entt::entity hitEntity, entt::entity abilityEntity)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			CallAllAbilityHitOrCritEvents(*world, characterEntity, hitEntity, abilityEntity, AbilitySystem::GetCritEvents());

		}, "CallAllCritEvents", MetaFunc::ExplicitParams<
		entt::entity, entt::entity, entt::entity>{}, "CharacterEntity", "HitEntity", "AbilityEntity").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	return metaType;
}

std::pair<std::optional<float>, bool> CE::AbilityFunctionality::ApplyInstantEffect(World& world, const CharacterComponent* castByCharacterData, entt::entity affectedEntity, AbilityEffect effect, bool doNotApplyColor)
{
	auto& reg = world.GetRegistry();
	auto characterComponent = reg.TryGet<CharacterComponent>(affectedEntity);
	if (characterComponent == nullptr)
	{
		LOG(LogAbilitySystem, Error, "Apply Effect - AffectedEntity {} is not a character.", entt::to_integral(affectedEntity));
		return { std::nullopt, false };
	}
	auto [base, current] = GetStat(effect.mStat, *characterComponent);

	if (effect.mFlatOrPercentage == FlatOrPercentage::Percentage)
	{
		effect.mAmount = effect.mAmount * 0.01f * base;
	}

	bool criticalHit{};
	if (effect.mIncreaseOrDecrease == IncreaseOrDecrease::Decrease)
	{
		if (effect.mStat == Stat::Health)
		{
			if (Random::Range<float>(0.f, 100.f) < effect.mCritChance)
			{
				// On Crit event
				criticalHit = true;
				effect.mAmount += effect.mAmount * effect.mCritIncrease * 0.01f;
			}
			float damageModifier = characterComponent->mCurrentReceivedDamageModifier;
			if (castByCharacterData != nullptr)
			{
				damageModifier += castByCharacterData->mCurrentDealtDamageModifier;
			}
			effect.mAmount += effect.mAmount * damageModifier * 0.01f;
			if (const auto isPlayer = reg.TryGet<PlayerComponent>(affectedEntity); isPlayer)
			{
				// On Getting Hit event
				AbilitySystem::CallBoundEventsWithNoExtraParams(world, affectedEntity, AbilitySystem::GetGettingHitEvents());
			}
			if (Math::AreFloatsEqual(characterComponent->mCurrentReceivedDamageModifier, -100.f))
			{
				// The character shouldn't take any damage.
				return { 0.f, criticalHit };
			}
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
	if (!doNotApplyColor)
	{
		auto effects = reg.TryGet<EffectsOnCharacterComponent>(affectedEntity);
		if (effects == nullptr)
		{
			LOG(LogAbilitySystem, Error, "Apply Effect - AffectedEntity {} does not have EffectsOnCharacterComponent attached.", entt::to_integral(affectedEntity));
			return { std::nullopt, false };
		}
		effects->mVisualEffects.push_back(VisualEffect{ GetEffectColor(effect.mStat, effect.mIncreaseOrDecrease) });
	}

	return { effect.mAmount, criticalHit };
}

void CE::AbilityFunctionality::ApplyDurationalEffect(World& world, const CharacterComponent* castByCharacterData, entt::entity affectedEntity, AbilityEffect effect, float duration, bool doNotApplyColor)
{
	const auto calculatedAmount = ApplyInstantEffect(world, castByCharacterData, affectedEntity, effect, doNotApplyColor);
	if (!calculatedAmount.first.has_value())
	{
		return;
	}

	auto& reg = world.GetRegistry();
	// We do a Get() here because we have already checked for this component in ApplyInstantEffect()
	auto& effects = reg.Get<EffectsOnCharacterComponent>(affectedEntity);

	effects.mDurationalEffects.push_back(DurationalEffect{ duration, 0.f, effect.mStat, calculatedAmount.first.value() });

	if (!doNotApplyColor)
	{
		effects.mVisualEffects.back().mDuration = duration;
	}
}

void CE::AbilityFunctionality::RevertDurationalEffect(CharacterComponent& characterComponent, const DurationalEffect& durationalEffect)
{
	float& currentStat = GetStat(durationalEffect.mStatAffected, characterComponent).second;
	currentStat -= durationalEffect.mAmount;
}

void CE::AbilityFunctionality::ApplyOverTimeEffect(World& world, const CharacterComponent*, entt::entity affectedEntity, AbilityEffect effect, float duration, int ticks)
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
	activeAbility->mCastByEntity = castBy;

	// Set the position and orientation.
	prefabTransform->SetLocalPosition(characterTransform->GetWorldPosition());
	prefabTransform->SetLocalOrientation(characterTransform->GetWorldOrientation());

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
		// Offset.
		const float radians = glm::radians(projectileComponent->mDirectionOffsetAngle); // Convert angle to radians
		const float radiansLeftOrRight = Random::Range<int32>(0, 2) == 0 ? radians : -radians;
		const glm::vec2 projectileDir = Math::RotateVec2ByAngleInRadians(characterDir, radiansLeftOrRight);
		// Set the velocity.
		prefabPhysicsBody->mLinearVelocity = glm::normalize(projectileDir) * projectileComponent->mSpeed;

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

entt::entity CE::AbilityFunctionality::SpawnProjectilePrefab(World& world, const Prefab& prefab, entt::entity castBy, const AssetHandle<Weapon>& weapon)
{
	entt::entity prefabEntity = SpawnAbilityPrefab(world, prefab, castBy);
	if (prefabEntity == entt::null)
	{
		LOG(LogAbilitySystem, Error, "The prefab does not have a PhysicsBody2DComponent attached.");
		return {};
	}
	auto& reg = world.GetRegistry();
	const Weapon& weaponRef = *weapon.Get();

	auto& projectileComponent = reg.Get<ProjectileComponent>(prefabEntity);
	projectileComponent.mSpeed = weaponRef.mProjectileSpeed;
	projectileComponent.mRange = weaponRef.mProjectileRange;
	projectileComponent.mPierceCount = weaponRef.mPierceCount;

	auto& physicsBodyComponent = reg.Get<PhysicsBody2DComponent>(prefabEntity);

	const auto characterTransform = reg.TryGet<TransformComponent>(castBy);
	const glm::vec2 characterDir = Math::QuatToDirectionXZ(characterTransform->GetWorldOrientation());
	physicsBodyComponent.mLinearVelocity = characterDir * projectileComponent.mSpeed;

	auto& transformComponent = reg.Get<TransformComponent>(prefabEntity);
	transformComponent.SetLocalScale(glm::vec3(weaponRef.mProjectileSize));

	auto& effectsComponent = reg.Get<AbilityEffectsComponent>(prefabEntity);
	effectsComponent.mEffects = weaponRef.mEffects;

	return prefabEntity;
}

entt::entity CE::AbilityFunctionality::SpawnProjectilePrefabFromWeaponInstance(World& world, const Prefab& prefab, entt::entity castBy, const WeaponInstance& weapon)
{
	if (!weapon.mRuntimeWeapon.has_value())
	{
		LOG(LogAbilitySystem, Error, "SpawnProjectilePrefabFromWeaponInstance - mRuntimeWeapon was never initialized.");
		return {};
	}
	entt::entity prefabEntity = SpawnAbilityPrefab(world, prefab, castBy);
	if (prefabEntity == entt::null)
	{
		LOG(LogAbilitySystem, Error, "The prefab does not have a PhysicsBody2DComponent attached.");
		return {};
	}
	auto& reg = world.GetRegistry();
	const Weapon& weaponRef = weapon.mRuntimeWeapon.value();

	auto& projectileComponent = reg.Get<ProjectileComponent>(prefabEntity);
	projectileComponent.mSpeed = weaponRef.mProjectileSpeed;
	projectileComponent.mRange = weaponRef.mProjectileRange;
	projectileComponent.mPierceCount = weaponRef.mPierceCount;

	auto& physicsBodyComponent = reg.Get<PhysicsBody2DComponent>(prefabEntity);

	const auto characterTransform = reg.TryGet<TransformComponent>(castBy);
	const glm::vec2 characterDir = Math::QuatToDirectionXZ(characterTransform->GetWorldOrientation());
	physicsBodyComponent.mLinearVelocity = characterDir * projectileComponent.mSpeed;

	auto& transformComponent = reg.Get<TransformComponent>(prefabEntity);
	transformComponent.SetLocalScale(glm::vec3(weaponRef.mProjectileSize));

	auto& effectsComponent = reg.Get<AbilityEffectsComponent>(prefabEntity);
	effectsComponent.mEffects = weaponRef.mEffects;

	return prefabEntity;
}

std::vector<entt::entity> CE::AbilityFunctionality::SpawnProjectilePrefabs(World& world, const Prefab& prefab, entt::entity castBy,
	const AssetHandle<Weapon>& weapon)
{
	const Weapon& weaponRef = *weapon.Get();
	if (weaponRef.mProjectileCount < 1)
	{
		LOG(LogAbilitySystem, Error, "Weapon {} Projectile Count is less than 1.");
		return {};
	}
	if (weaponRef.mProjectileCount == 1)
	{
		return { SpawnProjectilePrefab(world, prefab, castBy, weapon) };
	}

	// else more than 1 projectile
	auto& reg = world.GetRegistry();
	const float spreadInRadians = glm::radians(weaponRef.mSpread);
	const float angleBetweenProjectiles = spreadInRadians / static_cast<float>(weaponRef.mProjectileCount - 1);
	const auto characterTransform = reg.TryGet<TransformComponent>(castBy);
	const glm::vec2 characterDir = Math::QuatToDirectionXZ(characterTransform->GetWorldOrientation());
	const float directionInRadians = atan2(characterDir.y, characterDir.x);
	const float firstProjectileAngle = directionInRadians - spreadInRadians / 2.f;

	std::vector<entt::entity> projectilesVector{};
	for (int i = 0; i < weaponRef.mProjectileCount; i++)
	{
		auto projectile = SpawnProjectilePrefab(world, prefab, castBy, weapon);
		// Calculate and set the direction.
		auto& physicsBody = reg.Get<PhysicsBody2DComponent>(projectile);
		const auto projectileAngle = firstProjectileAngle + static_cast<float>(i) * angleBetweenProjectiles;
		physicsBody.mLinearVelocity = glm::vec2(cos(projectileAngle), sin(projectileAngle)) * weaponRef.mProjectileSpeed;
		projectilesVector.push_back(projectile);
	}
	return projectilesVector;
}

std::vector<entt::entity> CE::AbilityFunctionality::SpawnProjectilePrefabsFromWeaponInstance(World& world, const Prefab& prefab, entt::entity castBy, const WeaponInstance& weapon)
{
	if (!weapon.mRuntimeWeapon.has_value())
	{
		LOG(LogAbilitySystem, Error, "SpawnProjectilePrefabsFromWeaponInstance - mRuntimeWeapon was never initialized.");
		return {};
	}
	const Weapon& weaponRef = weapon.mRuntimeWeapon.value();
	if (weaponRef.mProjectileCount < 1)
	{
		LOG(LogAbilitySystem, Error, "SpawnProjectilePrefabsFromWeaponInstance - Projectile Count is less than 1.");
		return {};
	}
	if (weaponRef.mProjectileCount == 1)
	{
		return { SpawnProjectilePrefabFromWeaponInstance(world, prefab, castBy, weapon) };
	}

	// else more than 1 projectile
	auto& reg = world.GetRegistry();
	const float spreadInRadians = glm::radians(weaponRef.mSpread);
	const float angleBetweenProjectiles = spreadInRadians / static_cast<float>(weaponRef.mProjectileCount - 1);
	const auto characterTransform = reg.TryGet<TransformComponent>(castBy);
	const glm::vec2 characterDir = Math::QuatToDirectionXZ(characterTransform->GetWorldOrientation());
	const float directionInRadians = atan2(characterDir.y, characterDir.x);
	const float firstProjectileAngle = directionInRadians - spreadInRadians / 2.f;

	std::vector<entt::entity> projectilesVector{};
	for (int i = 0; i < weaponRef.mProjectileCount; i++)
	{
		auto projectile = SpawnProjectilePrefabFromWeaponInstance(world, prefab, castBy, weapon);
		// Calculate and set the direction.
		auto& physicsBody = reg.Get<PhysicsBody2DComponent>(projectile);
		const auto projectileAngle = firstProjectileAngle + static_cast<float>(i) * angleBetweenProjectiles;
		physicsBody.mLinearVelocity = glm::vec2(cos(projectileAngle), sin(projectileAngle)) * weaponRef.mProjectileSpeed;
		projectilesVector.push_back(projectile);
	}
	return projectilesVector;
}

bool CE::AbilityFunctionality::IncreasePierceCountAndReturnTrueIfExceeded(ProjectileComponent& projectileComponent)
{
	projectileComponent.mCurrentPierceCount++;
	return projectileComponent.mCurrentPierceCount > projectileComponent.mPierceCount;
}

bool CE::AbilityFunctionality::WasTheAbilityCastByAnEnemy(World& world, entt::entity entityToAffect, entt::entity abilityEntity)
{
	auto& reg = world.GetRegistry();
	const auto entityToAffectCharacterComponent = reg.TryGet<CharacterComponent>(entityToAffect);
	if (entityToAffectCharacterComponent == nullptr)
	{
		LOG(LogAbilitySystem, Error, "WasTheAbilityCastByAnEnemy - entityToAffect does not have a Character Component attached.");
		return false;
	}
	const auto activeAbilityComponent = reg.TryGet<ActiveAbilityComponent>(abilityEntity);
	if (activeAbilityComponent == nullptr)
	{
		LOG(LogAbilitySystem, Error, "WasTheAbilityCastByAnEnemy - entityToAffect does not have an Active Ability Component attached.");
		return false;
	}
	
	return entityToAffectCharacterComponent->mTeamId != activeAbilityComponent->mCastByCharacterData.mTeamId;
}

float CE::AbilityFunctionality::IncreaseValue1ByPercentageOfValue2(float value1, float value2, float percentage)
{
	value1 += percentage * 0.01f * value2;
	return value1;
}

bool CE::AbilityFunctionality::IsPointInsideCone2D(const glm::vec2 point, const glm::vec2 coneOrigin, const glm::vec2 coneDirection, const float coneAngle)
{
	const glm::vec2 pointDirNotNormalized = point - coneOrigin;
	if (coneDirection == glm::vec2(0.f) || pointDirNotNormalized == glm::vec2(0.f))
	{
		return false;
	}

	const glm::vec2 coneDir = glm::normalize(coneDirection);
	const glm::vec2 pointDir = glm::normalize(pointDirNotNormalized);

	// Calculate the cosine of the angle between the cone direction and the point direction
	const float cosAngle = glm::dot(coneDir, pointDir);

	// Calculate the cosine of the cone angle
	const float cosConeAngle = std::cos(glm::radians(coneAngle));

	// The point is inside the cone if the cosine of the angle between the cone direction and the point direction is greater than or equal to the cosine of the cone angle
	return cosAngle >= cosConeAngle;
}

void CE::AbilityFunctionality::RemoveWeaponAtIndex(World& world, entt::entity entity, int index)
{
	const auto abilities = world.GetRegistry().TryGet<AbilitiesOnCharacterComponent>(entity);
	if (abilities == nullptr)
	{
		LOG(LogAbilitySystem, Error, "RemoveWeaponAtIndex - entity {} does not have an Active Ability Component attached.", entt::to_integral(entity));
		return;
	}
	auto& weaponsVector = abilities->mWeaponsToInput;
	if (index >= static_cast<int>(weaponsVector.size()) || index < 0)
	{
		LOG(LogAbilitySystem, Error, "RemoveWeaponAtIndex - index out of range in entity {} with weapons vector with size {}.", index, entt::to_integral(entity), static_cast<int>(weaponsVector.size()));
		return;
	}
	const MetaType* scriptType = MetaManager::Get().TryGetType(weaponsVector[index].mWeaponAsset->mOnAbilityActivateScript.GetMetaData().GetName());
	if (scriptType != nullptr && world.GetRegistry().HasComponent(scriptType->GetTypeId(), entity))
	{
		world.GetRegistry().RemoveComponent(scriptType->GetTypeId(), entity);
	}
	weaponsVector.erase(weaponsVector.begin() + index);
}

void CE::AbilityFunctionality::AddWeaponToEnd(World& world, entt::entity entity, WeaponInstance& weapon)
{
	const auto abilities = world.GetRegistry().TryGet<AbilitiesOnCharacterComponent>(entity);
	if (abilities == nullptr)
	{
		LOG(LogAbilitySystem, Error, "AddWeapon - entity {} does not have an Active Ability Component attached.", entt::to_integral(entity));
		return;
	}
	abilities->mWeaponsToInput.push_back(weapon);
	abilities->mWeaponsToInput.back().InitializeRuntimeWeapon();
	// Add On Ability Activate script.
	const MetaType* scriptType = MetaManager::Get().TryGetType(weapon.mWeaponAsset->mOnAbilityActivateScript.GetMetaData().GetName());
	if (scriptType != nullptr && !world.GetRegistry().HasComponent(scriptType->GetTypeId(), entity))
	{
		world.GetRegistry().AddComponent(*scriptType, entity);
	}
}

void CE::AbilityFunctionality::ReplaceWeaponAtEnd(World& world, entt::entity entity, AssetHandle<Weapon>& weaponAsset)
{
	const auto abilities = world.GetRegistry().TryGet<AbilitiesOnCharacterComponent>(entity);
	if (abilities == nullptr)
	{
		LOG(LogAbilitySystem, Error, "ReplaceWeaponAtEnd - entity {} does not have an Active Ability Component attached.", entt::to_integral(entity));
		return;
	}
	auto& weaponsVector = abilities->mWeaponsToInput;
	if (weaponsVector.empty())
	{
		LOG(LogAbilitySystem, Error, "ReplaceWeaponAtEnd - no weapons found on entity {}.", entt::to_integral(entity));
		return;
	}
	auto& weaponInstance = abilities->mWeaponsToInput.back();
	// Remove old On Ability Activate script.
	const MetaType* scriptTypeOld = MetaManager::Get().TryGetType(weaponInstance.mWeaponAsset->mOnAbilityActivateScript.GetMetaData().GetName());
	if (scriptTypeOld != nullptr && world.GetRegistry().HasComponent(scriptTypeOld->GetTypeId(), entity))
	{
		world.GetRegistry().RemoveComponent(scriptTypeOld->GetTypeId(), entity);
	}

	weaponInstance.mWeaponAsset = weaponAsset;
	weaponInstance.InitializeRuntimeWeapon();
	// Add new On Ability Activate script.
	const MetaType* scriptTypeNew = MetaManager::Get().TryGetType(weaponInstance.mWeaponAsset->mOnAbilityActivateScript.GetMetaData().GetName());
	if (scriptTypeNew != nullptr && !world.GetRegistry().HasComponent(scriptTypeNew->GetTypeId(), entity))
	{
		world.GetRegistry().AddComponent(*scriptTypeNew, entity);
	}

	weaponInstance.ResetCooldownAndAmmo();
}

void CE::AbilityFunctionality::CallAllAbilityHitOrCritEvents(World& world, entt::entity characterEntity, entt::entity hitEntity, entt::entity abilityEntity, const std::vector<BoundEvent>& boundEvents)
{
	for (const BoundEvent& boundEvent : boundEvents)
	{
		entt::sparse_set* const storage = world.GetRegistry().Storage(boundEvent.mType.get().GetTypeId());

		if (storage == nullptr
			|| !storage->contains(characterEntity))
		{
			continue;
		}

		if (boundEvent.mIsStatic)
		{
			boundEvent.mFunc.get().InvokeUncheckedUnpacked(world, characterEntity, hitEntity, abilityEntity);
		}
		else
		{
			MetaAny component{ boundEvent.mType, storage->value(characterEntity), false };
			boundEvent.mFunc.get().InvokeUncheckedUnpacked(component, world, characterEntity, hitEntity, abilityEntity);
		}
	}
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
