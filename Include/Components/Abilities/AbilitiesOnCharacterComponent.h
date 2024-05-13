#pragma once
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include "Utilities/Imgui/ImguiInspect.h"
#include "Core/Input.h"
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class World;
	class Ability;
	
	struct AbilityInstance
	{
		AssetHandle<Ability> mAbilityAsset{};
		float mRequirementCounter{};
		int mChargesCounter{};

		std::vector<Input::KeyboardKey> mKeyboardKeys{};
		std::vector<Input::GamepadButton> mGamepadButtons{};

		void MakeAbilityReadyToBeActivated();

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

	struct WeaponInstance : AbilityInstance
	{
		float mTimeBetweenShotsCounter{};
		bool mAmmoConsumption = true;

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
		std::vector<AbilityInstance> mAbilitiesToInput{};
		std::vector<WeaponInstance> mWeaponsToInput{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		void OnBeginPlay(World&, entt::entity);
#ifdef EDITOR
		static void OnInspect(World& world, const std::vector<entt::entity>& entities);
#endif // EDITOR
		REFLECT_AT_START_UP(AbilitiesOnCharacterComponent);
	};

	template<class Archive>
	void serialize(Archive& ar, AbilityInstance& value)
	{
		ar(value.mAbilityAsset, value.mRequirementCounter, value.mChargesCounter, value.mKeyboardKeys, value.mGamepadButtons);
	}

	template<class Archive>
	void serialize(Archive& ar, WeaponInstance& value)
	{
		ar(value.mAbilityAsset, value.mRequirementCounter, value.mChargesCounter, value.mKeyboardKeys, value.mGamepadButtons, value.mTimeBetweenShotsCounter, value.mAmmoConsumption);
	}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, CE::AbilityInstance, var.DisplayWidget(); (void)name;)
IMGUI_AUTO_DEFINE_INLINE(template<>, CE::WeaponInstance, var.DisplayWidget(); (void)name;)
#endif // EDITOR