#pragma once
#include "Systems/System.h"

#include "Components/Particles/ParticleComponentTraits.h"

namespace CE
{
	class ParticleLifeTimeSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mFixedTickInterval = Particles::sParticleFixedTimeStep;
			traits.mPriority = static_cast<int>(TickPriorities::PreTick) + 1;
			return traits;
		}

	private:
		template<typename SpawnShapeType>
		static size_t UpdateEmitters(World& world, float dt, size_t& numOfEmittersFound);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleLifeTimeSystem);
	};
}
