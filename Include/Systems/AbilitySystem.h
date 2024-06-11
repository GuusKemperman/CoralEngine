#pragma once
#include "Core/Input.h"
#include "Systems/System.h"
#include "Utilities/Events.h"

namespace CE
{
	class AbilityFunctionality;
}

namespace CE
{
	class Input;
	class AbilitiesOnCharacterComponent;
	class CharacterComponent;
	struct AbilityInstance;
	struct WeaponInstance;

	class AbilitySystem final :
		public System
	{
	public:
		AbilitySystem();

		void Update(World& world, float dt) override;

		void UpdateAbilitiesVector(AbilitiesOnCharacterComponent& abilities, CharacterComponent& characterData, entt::entity entity, World& world, float dt);
		void UpdateWeaponsVector(AbilitiesOnCharacterComponent& abilities, CharacterComponent& characterData, entt::entity entity, World& world, float dt);

		// Checks for cooldowns/charges/requirements to see if an ability can be activated
		static bool CanAbilityBeActivated(const CharacterComponent& characterData, const AbilityInstance& ability);

		// Checks if the ability can be activated and if so
		// calls the On Ability Activate Event and resets cooldowns and charges.
		// Returns whether the ability was activated.
		static bool ActivateAbility(World& world, entt::entity castBy, CharacterComponent& characterData, AbilityInstance& ability);

		// Checks the same things CanAbilityBeActivated() plus extra weapon-specific things.
		static bool CanWeaponBeActivated(const CharacterComponent& characterData, const WeaponInstance& weapon);

		// Functions the same as ActivateAbility() plus extra weapon-specific things.
		static bool ActivateWeapon(World& world, entt::entity castBy, CharacterComponent& characterData, WeaponInstance& weapon);

		// Checks all the provided keyboard keys based on the function provided and returns true at the first input found, otherwise returns false.
		template<bool (Input::* Func)(Input::KeyboardKey, bool) const>
		static bool CheckKeyboardInput(const std::vector<Input::KeyboardKey>& keys);

		// Checks all the provided gamepad buttons based on the function provided and returns true at the first input found, otherwise returns false.
		template<bool (Input::* Func)(int, Input::GamepadButton, bool) const>
		static bool CheckGamepadInput(const std::vector<Input::GamepadButton>& buttons, int playerID);

		// Call all bound events of a type based on the vector of bound events passed - only events that have the based parameters (world and owner entity).
		static void CallBoundEventsWithNoExtraParams(World& world, entt::entity castBy, const std::vector<CE::BoundEvent>& boundEvents);

		static const std::vector<CE::BoundEvent>& GetEnemyKilledEvents() { return sEnemyKilledEvents; }
		static const std::vector<CE::BoundEvent>& GetGettingHitEvents() { return sGettingHitEvents; }
		static const std::vector<CE::BoundEvent>& GetCritEvents() { return sCritEvents; }

	private:
		static inline std::vector<CE::BoundEvent> sAbilityActivateEvents;
		static inline std::vector<CE::BoundEvent> sReloadCompletedEvents;
		static inline std::vector<CE::BoundEvent> sEnemyKilledEvents;
		static inline std::vector<CE::BoundEvent> sGettingHitEvents;
		static inline std::vector<CE::BoundEvent> sCritEvents;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilitySystem);
	};

	template <bool(Input::*Func)(Input::KeyboardKey, bool) const>
	bool AbilitySystem::CheckKeyboardInput(const std::vector<Input::KeyboardKey>& keys)
	{
		const auto& input = Input::Get();
		for (auto& key : keys)
		{
			if ((input.*Func)(key, true))
			{
				return true;
			}
		}
		return false;
	}

	template <bool(Input::*Func)(int, Input::GamepadButton, bool) const>
	bool AbilitySystem::CheckGamepadInput(const std::vector<Input::GamepadButton>& buttons, int playerID)
	{
		const auto& input = Input::Get();
		for (auto& button : buttons)
		{
			if ((input.*Func)(playerID, button, true))
			{
				return true;
			}
		}
		return false;
	}
}
