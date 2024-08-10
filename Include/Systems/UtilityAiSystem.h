#pragma once
#include "Systems/System.h"

namespace CE
{
	class AITickSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mPriority = static_cast<int>(TickPriorities::PreTick) - 1;
			return traits;
		}

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
			traits.mPriority = static_cast<int>(TickPriorities::PreTick) + 1;
			return traits;
		}

	private:
		template<typename EventT>
		static void CallTransitionEvent(const EventT& event, const MetaType* type, World& world, entt::entity owner);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AIEvaluateSystem);
	};
}
