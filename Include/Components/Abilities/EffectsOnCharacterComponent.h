#pragma once

#include "Meta/MetaReflect.h"
#include "Utilities/AbilityFunctionality.h"
#include "Utilities/Imgui/ImguiInspect.h"

namespace Engine
{
	// Effects that get reverted after the specified duration.
	struct DurationalEffect
	{
		// The amount of time after which the effect will get reverted.
		float mDuration{};

		// How long the effect has been active for.
		float mDurationTimer{};

		// Which stat has been modified.
		AbilityFunctionality::Stat mStatAffected{};

		// The amount with which the stat has been modified and will be inversely applied with.
		float mAmount{}; 

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

	// Effect that gets applied every X seconds (like a DOT).
	struct OverTimeEffect
	{
		// Interval at which the effect will be applied.
		float mTickDuration{};

		// How long it has been since the last tick.
		float mDurationTimer{};

		// How many times the effect should be applied.
		int mNumberOfTicks{};

		// How many ticks have passed.
		int mTicksCounter{};

		// The effect to apply.
		AbilityFunctionality::EffectSettings mEffectSettings{};

		// This is needed only if the effect is health decrease (damage).
		// It needs to be stored because the dealt damage modifer of the character
		// that cast the ability might have changed in the meantime,
		// but we only want to apply the initial value at the moment the ability was cast.
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

	// For player feedback - for now it is just a color applied to the mesh of the character.
	struct VisualEffect
	{
		// The visual effect is a color applied to the mesh of the character.
		glm::vec3 mColor{};

		// How long the effect should be displayed for.
		float mDuration = 0.3f; // default value for instant effects

		// How long the effect has been active for.
		float mDurationTimer{};

		bool operator==(const VisualEffect& other) const;
		bool operator!=(const VisualEffect& other) const;

#ifdef EDITOR
		void DisplayWidget();
#endif // EDITOR

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(VisualEffect);
	};

	// effects of abilities that are not instant
	class EffectsOnCharacterComponent
	{
	public:
		std::vector<DurationalEffect> mDurationalEffects{};
		std::vector<OverTimeEffect> mOverTimeEffects{};
		std::vector<VisualEffect> mVisualEffects{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(EffectsOnCharacterComponent);
	};

	template<class Archive>
	void serialize([[maybe_unused]] Archive& ar, [[maybe_unused]] const DurationalEffect& value)
	{
		// We don't need to actually serialize it, but otherwise we get a compilation error
	}

	template<class Archive>
	void serialize([[maybe_unused]] Archive& ar, [[maybe_unused]] const OverTimeEffect& value)
	{
		// We don't need to actually serialize it, but otherwise we get a compilation error
	}

	template<class Archive>
	void serialize([[maybe_unused]] Archive& ar, [[maybe_unused]] const VisualEffect& value)
	{
		// We don't need to actually serialize it, but otherwise we get a compilation error
	}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, Engine::DurationalEffect, var.DisplayWidget(); (void)name;)
IMGUI_AUTO_DEFINE_INLINE(template<>, Engine::OverTimeEffect, var.DisplayWidget(); (void)name;)
IMGUI_AUTO_DEFINE_INLINE(template<>, Engine::VisualEffect, var.DisplayWidget(); (void)name;)
#endif // EDITOR
