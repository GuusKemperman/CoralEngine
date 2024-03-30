#pragma once
#include "Systems/System.h"

namespace CE
{
	class UpdateCameraMatricesSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mPriority = static_cast<int>(TickPriorities::PostTick);
			traits.mShouldTickBeforeBeginPlay = true;
			traits.mShouldTickWhilstPaused = true;
			return traits;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UpdateCameraMatricesSystem);
	};
}