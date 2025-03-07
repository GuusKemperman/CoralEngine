#pragma once
#include "Systems/System.h"

namespace Game
{
	class EnvironmentGeneratorSystem final :
		public CE::System
	{
	public:
		void Update(CE::World& world, float dt) override;

		CE::SystemStaticTraits GetStaticTraits() const override
		{
			CE::SystemStaticTraits traits{};

			// We do debug rendering even
			// whilst paused or before begin play
			traits.mShouldTickBeforeBeginPlay = true;
			traits.mShouldTickWhilstPaused = true;
			return traits;
		}

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(EnvironmentGeneratorSystem);
	};
}
