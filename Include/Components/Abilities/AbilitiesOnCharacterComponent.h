#pragma once
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include "Assets/Ability/Weapon.h"
#include "Utilities/Imgui/ImguiInspect.h"
#include "Core/Input.h"
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class World;
	class Ability;
	class Weapon;
	
	struct AbilityInstance
	{
		AssetHandle<Ability> mAbilityAsset{};
		float mRequirementCounter{};
		int mChargesCounter{};

		std::vector<Input::KeyboardKey> mKeyboardKeys{};
		std::vector<Input::GamepadButton> mGamepadButtons{};

		void ResetCooldownAndCharges();

		bool operator==(const AbilityInstance& other) const;
		bool operator!=(const AbilityInstance& other) const;

#ifdef EDITOR
		void DisplayWidget();
#endif // EDITOR

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilityInstance);
	};

	// Would be nice to inherit from Ability instance,
	// but WeaponInstance needs to hold a Weapon asset and not an Ability asset.
	struct WeaponInstance
	{
		AssetHandle<Weapon> mWeaponAsset{};
		std::optional<Weapon> mRuntimeWeapon{};
		float mReloadCounter{};
		int mAmmoCounter{};
		float mShotDelayCounter{};
		int mShotsAccumulated{};
		bool mAmmoConsumption = true;

		std::vector<Input::KeyboardKey> mKeyboardKeys{};
		std::vector<Input::GamepadButton> mGamepadButtons{};

		std::vector<Input::KeyboardKey> mReloadKeyboardKeys{};
		std::vector<Input::GamepadButton> mReloadGamepadButtons{};

		void ResetCooldownAndAmmo();
		Weapon* InitializeRuntimeWeapon();

		bool operator==(const WeaponInstance& other) const;
		bool operator!=(const WeaponInstance& other) const;

#ifdef EDITOR
		void DisplayWidget();
#endif // EDITOR

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(WeaponInstance);
	};

	class AbilitiesOnCharacterComponent
	{
	public:
		void OnBeginPlay(World&, entt::entity);

#ifdef EDITOR
		void OnInspect(World& world, entt::entity owner);
#endif // EDITOR

		std::vector<AbilityInstance> mAbilitiesToInput{};
		std::vector<WeaponInstance> mWeaponsToInput{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();

		REFLECT_AT_START_UP(AbilitiesOnCharacterComponent);
	};

	template<class Archive>
	void serialize(Archive& ar, AbilityInstance& value)
	{
		ar(value.mAbilityAsset, value.mRequirementCounter, value.mChargesCounter, value.mKeyboardKeys, value.mGamepadButtons);
	}

	static constexpr uint32 sWeaponInstanceVersion = 1245960345u;

	template<class Archive>
	void load(Archive& ar, WeaponInstance& value)
	{

		uint32 version{};
		ar(value.mWeaponAsset, version);
		if (version == sWeaponInstanceVersion)
		{
			ar(value.mKeyboardKeys, value.mGamepadButtons, value.mReloadKeyboardKeys, value.mReloadGamepadButtons);
		}
		else
		{
			float floatDummy{};
			int intDummy{};
			bool boolDummy{};
			ar(intDummy, floatDummy, boolDummy, value.mKeyboardKeys, value.mGamepadButtons);
		}
	}

	template<class Archive>
	void save(Archive & ar, const WeaponInstance & value)
	{
		ar(value.mWeaponAsset, sWeaponInstanceVersion, value.mKeyboardKeys, value.mGamepadButtons, value.mReloadKeyboardKeys, value.mReloadGamepadButtons);
	}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, CE::AbilityInstance, var.DisplayWidget(); (void)name;)
IMGUI_AUTO_DEFINE_INLINE(template<>, CE::WeaponInstance, var.DisplayWidget(); (void)name;)
#endif // EDITOR