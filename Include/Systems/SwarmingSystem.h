#pragma once
#include "Systems/System.h"

namespace CE
{
	// Moves the agents through the flowfield
	class SwarmingAgentSystem final
		: public System
	{
	public:
		void Update(World& world, float dt) override;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(SwarmingAgentSystem);
	};

	// Updates the flowfield
	class SwarmingTargetSystem final
		: public System
	{
	public:
		void Update(World& world, float dt) override;

		void Render(const World& world) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mFixedTickInterval = .7f;
			return traits;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(SwarmingTargetSystem);
	};
}
