#pragma once
#include "Systems/System.h"

#include "Components/Particles/ParticleComponentTraits.h"

namespace Engine
{
	class ParticleScaleOverTimeSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mFixedTickInterval = Particles::sParticleFixedTimeStep;
			traits.mPriority = static_cast<int>(TickPriorities::PreTick);
			return traits;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleScaleOverTimeSystem);
	};
}