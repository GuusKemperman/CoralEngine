#pragma once
#ifdef EDITOR
#include "Meta/MetaReflect.h"
#include "Components/Component.h"

namespace Engine
{
	class World;

	class EventTestingComponent
	{
	public:
		void OnTick(World& world, entt::entity owner, float dt);

		void OnFixedTick(World& world, entt::entity owner);

		entt::entity mOwner{ entt::null };

		uint32 mNumOfTicks{};
		uint32 mNumOfFixedTicks{};
		uint32 mTotalNumOfEventsCalled{};

		World* mLastReceivedWorld{};
		entt::entity mLastReceivedOwner{ entt::null };

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(EventTestingComponent);
	};

	template<>
	struct AlwaysPassComponentOwnerAsFirstArgumentOfConstructor<EventTestingComponent>
	{
		static constexpr bool sValue = true;
	};
}
#endif // EDITOR