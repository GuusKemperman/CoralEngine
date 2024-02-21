#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	static inline constexpr int TickPriorityStepSize = 100;

	enum class TickPriorities
	{
		PrePhysics = TickPriorityStepSize * 4,
		Physics = TickPriorityStepSize * 3,
		PostPhysics = TickPriorityStepSize * 2,
		PreTick = TickPriorityStepSize,
		Tick = 0,
		PostTick = -TickPriorityStepSize,
		PreRender = -TickPriorityStepSize * 2,
		Render = -TickPriorityStepSize * 3,
		PostRender = -TickPriorityStepSize * 4
	};


	struct SystemStaticTraits
	{
		// If no fixed tick interval is provided, 
		// the system will be ticked every frame.
		// Fixed tick systems are always executed before tick
		std::optional<float> mFixedTickInterval{};
		
		// Higher priorities get ticked first.
		// Systems with the same priority MAY be exectuted in parallel.
		int mPriority = static_cast<int>(TickPriorities::Tick);

		bool mShouldTickBeforeBeginPlay{};

		bool mShouldTickWhilstPaused{};
	};

	class World;

	class System
	{
	public:
		virtual ~System() = default;

		virtual void Update([[maybe_unused]] World& world, [[maybe_unused]] float dt) {};
		virtual void Render([[maybe_unused]] const World& world) {};

		virtual SystemStaticTraits GetStaticTraits() const { return {}; };

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(System);
	};
}