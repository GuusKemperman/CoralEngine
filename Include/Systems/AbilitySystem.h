#pragma once
#include "Systems/System.h"

namespace CE
{
	class CharacterComponent;
	struct AbilityInstance;

	class AbilitySystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		// Checks for cooldowns/charges/requirements to see if an ability can be activated
		static bool CanAbilityBeActivated(const CharacterComponent& characterData, const AbilityInstance& ability);

		// Checks if the ability can be activated and if so
		// calls the On Ability Activate Event and resets cooldowns and charges.
		// Returns whether the ability was activated.
		static bool ActivateAbility(World& world, entt::entity castBy, CharacterComponent& characterData, AbilityInstance& ability);

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilitySystem);
	};
}
