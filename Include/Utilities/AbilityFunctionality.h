#pragma once

#include "Events.h"
#include "Assets/Core/AssetHandle.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Abilities/AbilityEffectsComponent.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class AbilitiesOnCharacterComponent;
	struct WeaponInstance;
	class ProjectileComponent;
	struct DurationalEffect;
	class Prefab;
	class CharacterComponent;
	class World;
	class Weapon;

	class AbilityFunctionality
	{
	public:
		static std::pair<std::optional<float>, bool> ApplyInstantEffect(World& world, const CharacterComponent* castByCharacterData, entt::entity affectedEntity, AbilityEffect effect, bool doNotApplyColor = false);
		static void ApplyDurationalEffect(World& world, const CharacterComponent* castByCharacterData, entt::entity affectedEntity, AbilityEffect effect, float duration = 0.f, bool doNotApplyColor = false);
		static void RevertDurationalEffect(CharacterComponent& characterComponent, const DurationalEffect& durationalEffect);
		static void ApplyOverTimeEffect(World& world, const CharacterComponent* castByCharacterData, entt::entity affectedEntity, AbilityEffect effect, float duration = 0.f, int ticks = 1);
		static entt::entity SpawnAbilityPrefab(World& world, const Prefab& prefab, entt::entity castBy);
		static entt::entity SpawnProjectilePrefab(World& world, const Prefab& prefab, entt::entity castBy, const AssetHandle<Weapon>& weapon);
		static entt::entity SpawnProjectilePrefabFromWeaponInstance(World& world, const Prefab& prefab, entt::entity castBy, const WeaponInstance& weapon);
		static std::vector<entt::entity> SpawnProjectilePrefabs(World& world, const Prefab& prefab, entt::entity castBy, const AssetHandle<Weapon>& weapon);
		static std::vector<entt::entity> SpawnProjectilePrefabsFromWeaponInstance(World& world, const Prefab& prefab, entt::entity castBy, const WeaponInstance& weapon);
		static bool IncreasePierceCountAndReturnTrueIfExceeded(ProjectileComponent& projectileComponent);
		static bool WasTheAbilityCastByAnEnemy(World& world, entt::entity entityToAffect, entt::entity abilityEntity);
		static float IncreaseValue1ByPercentageOfValue2(float value1, float value2, float percentage);
		static bool IsPointInsideCone2D(glm::vec2 point, glm::vec2 coneOrigin, const glm::vec2 coneDirection, float coneAngle);
		static void RemoveWeaponAtIndex(World& world, entt::entity entity, int index);
		static void AddWeaponToEnd(World& world, entt::entity entity, WeaponInstance& weapon);
		static void ReplaceWeaponAtEnd(World& world, entt::entity entity, AssetHandle<Weapon>& weaponAsset);
		static void CallAllAbilityHitOrCritEvents(World& world, entt::entity characterEntity, entt::entity hitEntity, entt::entity abilityEntity, const std::vector<BoundEvent>& boundEvents);

	private:
		static std::pair<float&, float&> GetStat(Stat stat, CharacterComponent& characterComponent);
		static glm::vec3 GetEffectColor(Stat stat, IncreaseOrDecrease increaseOrDecrease);

		struct PairOfOptionalAndValueHash // to use a key in std::unordered_map
		{
			template <class T1, class T2>
			std::size_t operator() (const std::pair<T1, T2>& pair) const
			{
				auto hash1 = pair.first ? std::hash<T1>{}(*pair.first) : 0; // Hash of std::optional of first value
				auto hash2 = std::hash<T2>{}(pair.second); // Hash of second value
				return hash1 ^ (hash2 << 1); // Combine hashes
			}
		};

		inline static std::unordered_map<std::pair<std::optional<Stat>, IncreaseOrDecrease>, glm::vec3, PairOfOptionalAndValueHash>
			sDefaultEffectColors = {
		{{std::nullopt, IncreaseOrDecrease::Increase}, {1.f, 1.f, 1.f}},               // Default increase - white
		{{std::nullopt, IncreaseOrDecrease::Decrease}, {0.05f, 0.05f, 0.05f}},         // Default decrease - black
		{{Stat::Health, IncreaseOrDecrease::Increase}, {0.f, 1.f, 0.f}},                    // green
		{{Stat::Health, IncreaseOrDecrease::Decrease}, {1.f, 0.f, 0.f}},                    // red
		{{Stat::MovementSpeed, IncreaseOrDecrease::Increase}, {1.f, 0.5f, 0.f}},            // orange
		{{Stat::MovementSpeed, IncreaseOrDecrease::Decrease}, {0.25f, 0.25f, 1.f}},         // blue
		{{Stat::DealtDamageModifier, IncreaseOrDecrease::Increase}, {0.75f, 0.25f, 0.75f}}, // purple
		{{Stat::ReceivedDamageModifier, IncreaseOrDecrease::Increase}, {1.f, 1.f, 0.25f}},  // yellow
		};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilityFunctionality);
	};
}
