#pragma once
#include "Systems/System.h"

namespace CE
{
	class PhysicsSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		void Render(const World& world, RenderCommandQueue& commandQueue) const override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mPriority = static_cast<int>(TickPriorities::Physics);
			traits.mFixedTickInterval = 1 / 60.0f;
			traits.mShouldTickBeforeBeginPlay = true;
			traits.mShouldTickWhilstPaused = true;
			return traits;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PhysicsSystem);
	};
}
