#pragma once
#include "Components/SpawnerComponent.h"
#include "Systems/System.h"

namespace Game
{
	class SpawnerSystem final :
		public CE::System
	{
	public:
		void Update(CE::World& world, float dt) override;

		CE::SystemStaticTraits GetStaticTraits() const override
		{
			CE::SystemStaticTraits traits{};
			traits.mFixedTickInterval = 1.0f;
			return traits;
		}

	private:
		// Gets called on finishing a wave (does not get called for the last wave).
		void AddComponentOnWaveFinished(CE::World& world, SpawnerComponent::Wave& wave, entt::entity spawnerEntity);

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(SpawnerSystem);
	};
}
