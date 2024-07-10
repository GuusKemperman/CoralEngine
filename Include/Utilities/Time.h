#pragma once
#include "Meta/MetaReflect.h"
#include "Utilities/Imgui/ImguiInspect.h"

namespace CE
{
	struct Cooldown
	{
		bool IsReady(float dt);
		void Reset();

#ifdef EDITOR
		void DisplayWidget(const std::string& name);
#endif // EDITOR

		bool operator==(const Cooldown& other) const;
		bool operator!=(const Cooldown& other) const;

		float mCooldown = 1.0f;
		float mAmountOfTimePassed{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Cooldown);
	};

	struct Timer
	{
		Timer();

		float GetSecondsElapsed() const;

		void Reset();
		std::chrono::high_resolution_clock::time_point mStart{};
	};
}

namespace cereal
{
	class BinaryOutputArchive;
	class BinaryInputArchive;

	void save(BinaryOutputArchive& ar, const CE::Cooldown& value);
	void load(BinaryInputArchive& ar, CE::Cooldown& value);
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, CE::Cooldown, var.DisplayWidget(name);)
#endif // EDITOR