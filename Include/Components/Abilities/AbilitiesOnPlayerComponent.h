#pragma once
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include "Assets/Ability.h"
#include "Utilities/Imgui/ImguiInspect.h"
#include "Core/Input.h"
#include "Meta/MetaReflect.h"

namespace Engine
{
	class Ability;
	
	struct AbilityInstanceWithInputs
	{
		std::shared_ptr<const Ability> mAbilityAsset{};
		float mRequirementCounter{};
		int mChargesCounter{};

		std::vector<Input::KeyboardKey> mKeyboardKeys{};
		std::vector<Input::GamepadButton> mGamepadButtons{};

		bool operator==(const AbilityInstanceWithInputs& other) const;
		bool operator!=(const AbilityInstanceWithInputs& other) const;

#ifdef EDITOR
		void DisplayWidget();
#endif // EDITOR

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilityInstanceWithInputs);
	};

	class AbilitiesOnPlayerComponent
	{
	public:
		std::vector<AbilityInstanceWithInputs> mAbilitiesToInput{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilitiesOnPlayerComponent);
	};

	template<class Archive>
	void save(Archive& ar, const AbilityInstanceWithInputs& value)
	{
		ar(value.mAbilityAsset, value.mRequirementCounter, value.mChargesCounter, value.mKeyboardKeys, value.mGamepadButtons);
	}

	template<class Archive>
	void load(Archive& ar, AbilityInstanceWithInputs& value)
	{
		ar(value.mAbilityAsset, value.mRequirementCounter, value.mChargesCounter, value.mKeyboardKeys, value.mGamepadButtons);
	}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, Engine::AbilityInstanceWithInputs, var.DisplayWidget(); (void)name;)
#endif // EDITOR