#pragma once
#include "Components/Pathfinding/SwarmingTargetComponent.h"
#include "Systems/System.h"
#include "Utilities/Time.h"

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
		~SwarmingTargetSystem();

		void Update(World& world, float dt) override;

		void Render(const World& world) override;

	private:
		Cooldown mStartNewThreadCooldown{ .5f };

		entt::entity mPendingForEntity{};
		bool mIsPendingReady{};

		SwarmingTargetComponent::FlowField mPendingFlowField{};

		// Large buffers are stored in the system,
		// so we can reuse the buffer.
		// There can only be one flowfield by design,
		// which allows us to place the data in the system.
		std::vector<glm::vec2> mSmoothedDirections{};

		// char because std::vector<bool> is slower
		std::vector<char> mPendingIsBlocked{};
		std::vector<int> mPendingOpenIndices{};
		std::vector<float> mPendingDistanceField{};

		std::thread mPendingThread{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(SwarmingTargetSystem);
	};
}
