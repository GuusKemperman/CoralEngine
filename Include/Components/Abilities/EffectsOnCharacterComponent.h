#pragma once

#include "Utilities/AbilityFunctionality.h"
#include "Utilities/Imgui/ImguiInspect.h"

namespace Engine
{
	// effects that get reverted after the specified duration
	struct DurationalEffect
	{
		float mDuration{}; // the amount of time after which the effect will get reverted
		float mCurrentDuration{}; // the amount of time after which the effect will get reverted
		AbilityFunctionality::Stat mStatAffected{}; // which stat has been modified
		float mAmount{}; // the amount with which the stat has been modified and will be inverse reverted

		bool operator==(const DurationalEffect& other) const;
		bool operator!=(const DurationalEffect& other) const;

#ifdef EDITOR
		void DisplayWidget();
#endif // EDITOR

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(DurationalEffect);
	};

	// effects of abilities that are not instant
	class EffectsOnCharacterComponent
	{
	public:
		std::vector<DurationalEffect> mDurationalEffects{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(EffectsOnCharacterComponent);
	};

	template<class Archive>
	void serialize([[maybe_unused]] Archive& ar, [[maybe_unused]] const DurationalEffect& value)
	{
	}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, Engine::DurationalEffect, var.DisplayWidget(); (void)name;)
#endif // EDITOR