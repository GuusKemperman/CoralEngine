#pragma once
#include "Systems/System.h"

namespace CE
{
	class AbilitiesOnCharacterComponent;
	class CharacterComponent;
	struct AbilityInstance;
	struct WeaponInstance;

	class AbilitySystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		void IterateAbilitiesVector(AbilitiesOnCharacterComponent& abilities, CharacterComponent& characterData, entt::entity entity, World& world, float dt);
		void IterateWeaponsVector(AbilitiesOnCharacterComponent& abilities, CharacterComponent& characterData, entt::entity entity, World& world, float dt);

		// Checks for cooldowns/charges/requirements to see if an ability can be activated
		static bool CanAbilityBeActivated(const CharacterComponent& characterData, const AbilityInstance& ability);

		// Checks if the ability can be activated and if so
		// calls the On Ability Activate Event and resets cooldowns and charges.
		// Returns whether the ability was activated.
		static bool ActivateAbility(World& world, entt::entity castBy, CharacterComponent& characterData, AbilityInstance& ability);

		// Checks the same things CanAbilityBeActivated() plus extra weapon specific things.
		static bool CanWeaponBeActivated(const CharacterComponent& characterData, const WeaponInstance& weapon);

		//  
		// Returns whether the ability was activated.
		static bool ActivateWeapon(World& world, entt::entity castBy, CharacterComponent& characterData, WeaponInstance& weapon);

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilitySystem);
	};
}
