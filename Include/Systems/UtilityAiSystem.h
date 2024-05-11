#pragma once
#include "Systems/System.h"
#include "Utilities/Events.h"

namespace CE
{
	class AITickSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AITickSystem);
	};

	class AIEvaluateSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mPriority = static_cast<int>(TickPriorities::PreTick);
			traits.mFixedTickInterval = 0.2f;
			return traits;
		}

	private:
		template<typename EventT>
		static void CallTransitionEvent(const EventT& event, const MetaType* type, World& world, entt::entity owner);

		std::vector<BoundEvent> mBoundEvaluateEvents = GetAllBoundEvents(sAIEvaluateEvent);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AIEvaluateSystem);
	};
}
