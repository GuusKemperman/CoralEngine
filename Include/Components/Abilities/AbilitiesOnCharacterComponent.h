#pragma once
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include "Utilities/Imgui/ImguiInspect.h"
#include "Core/Input.h"
#include "Meta/MetaReflect.h"

namespace Engine
{
	class World;
	class Ability;
	
	struct AbilityInstance
	{
		std::shared_ptr<const Ability> mAbilityAsset{};
		float mRequirementCounter{};
		int mChargesCounter{};

		std::vector<Input::KeyboardKey> mKeyboardKeys;
		std::vector<Input::GamepadButton> mGamepadButtons;

		bool operator==(const AbilityInstance& other) const;
		bool operator!=(const AbilityInstance& other) const;

#ifdef EDITOR
		void DisplayWidget();
#endif // EDITOR

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilityInstance);
	};

	class AbilitiesOnCharacterComponent
	{
	public:
		bool mIsPlayer = true;
		std::vector<AbilityInstance> mAbilitiesToInput;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
#ifdef EDITOR
		static void OnInspect(World& world, const std::vector<entt::entity>& entities);
#endif // EDITOR
		REFLECT_AT_START_UP(AbilitiesOnCharacterComponent);
	};

	template<class Archive>
	void save(Archive& ar, const AbilityInstance& value)
	{
		ar(value.mAbilityAsset, value.mRequirementCounter, value.mChargesCounter, value.mKeyboardKeys, value.mGamepadButtons);
	}

	template<class Archive>
	void load(Archive& ar, AbilityInstance& value)
	{
		ar(value.mAbilityAsset, value.mRequirementCounter, value.mChargesCounter, value.mKeyboardKeys, value.mGamepadButtons);
	}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, Engine::AbilityInstance, var.DisplayWidget(); (void)name;)
#endif // EDITOR