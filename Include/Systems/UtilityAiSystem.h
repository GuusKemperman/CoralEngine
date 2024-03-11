#pragma once
#include "Systems/System.h"

namespace Engine
{
	class UtilityAiSystem final : public System
	{
	public:
		void Update(World& world, float dt) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mPriority = static_cast<int>(TickPriorities::PostPhysics);
			traits.mFixedTickInterval = 0.2f;
			traits.mShouldTickBeforeBeginPlay = true;
			return traits;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UtilityAiSystem);
	};
}
