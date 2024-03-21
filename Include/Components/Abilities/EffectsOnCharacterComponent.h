#pragma once

#include "Meta/MetaReflect.h"
#include "Utilities/AbilityFunctionality.h"
#include "Utilities/Imgui/ImguiInspect.h"

namespace Engine
{
	// effects that get reverted after the specified duration
	struct DurationalEffect
	{
		float mDuration{}; // the amount of time after which the effect will get reverted
		float mDurationTimer{}; // the amount of time after which the effect will get reverted
		AbilityFunctionality::Stat mStatAffected{}; // which stat has been modified
		float mAmount{}; // the amount with which the stat has been modified and will be inverse reverted

		bool operator==(const DurationalEffect& other) const;
		bool operator!=(const DurationalEffect& other) const;

#ifdef EDITOR
		void DisplayWidget();
#endif // EDITOR

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(DurationalEffect);
	};

	// effect that gets applied every X seconds (like a DOT)
	struct OverTimeEffect
	{
		float mDuration{}; // tick duration
		float mDurationTimer{}; // how long it has been since the last tick
		int mTicks{}; // how many times the effect should be applied
		int mTicksCounter{}; // how many ticks have passed
		AbilityFunctionality::EffectSettings mEffectSettings{}; // effect to apply
		// this is needed only if the effect is health decrease (damage)
		// it needs to be stored because the dealt damage modifer of the character that cast the ability
		// might have changed in the meantime, but we only want to apply the initial value when the ability was cast
		float mDealtDamageModifierOfCastByCharacter{};

		bool operator==(const OverTimeEffect& other) const;
		bool operator!=(const OverTimeEffect& other) const;

#ifdef EDITOR
		void DisplayWidget();
#endif // EDITOR

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(OverTimeEffect);
	};

	// effects of abilities that are not instant
	class EffectsOnCharacterComponent
	{
	public:
		std::vector<DurationalEffect> mDurationalEffects{};
		std::vector<OverTimeEffect> mOverTimeEffects{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(EffectsOnCharacterComponent);
	};

	template<class Archive>
	void serialize([[maybe_unused]] Archive& ar, [[maybe_unused]] const DurationalEffect& value)
	{
	}

	template<class Archive>
	void serialize([[maybe_unused]] Archive& ar, [[maybe_unused]] const OverTimeEffect& value)
	{
	}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, Engine::DurationalEffect, var.DisplayWidget(); (void)name;)
IMGUI_AUTO_DEFINE_INLINE(template<>, Engine::OverTimeEffect, var.DisplayWidget(); (void)name;)
#endif // EDITOR
