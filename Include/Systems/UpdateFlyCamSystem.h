#pragma once
#include "Systems/System.h"

namespace Engine
{
	class UpdateFlyCamSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mShouldTickBeforeBeginPlay = true;
			traits.mShouldTickWhilstPaused = true;
			return traits;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UpdateFlyCamSystem);
	};
}