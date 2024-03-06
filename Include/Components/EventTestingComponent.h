#pragma once
#ifdef EDITOR
#include "Meta/MetaReflect.h"
#include "Components/Component.h"

namespace Engine
{
	class World;

	class EmptyEventTestingComponent
	{
	public:
		static void OnConstruct(World& world, entt::entity owner);

		static void OnBeginPlay(World& world, entt::entity owner);

		static void OnTick(World& world, entt::entity owner, float dt);

		static void OnFixedTick(World& world, entt::entity owner);

		static uint32 GetValue(Name valueName);

		static inline uint32 sNumOfConstructs{};
		static inline uint32 sNumOfBeginPlays{};
		static inline uint32 sNumOfTicks{};
		static inline uint32 sNumOfFixedTicks{};

		static void Reset();

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(EmptyEventTestingComponent);
	};

	class EventTestingComponent
	{
	public:
		void OnConstruct(World& world, entt::entity owner);

		void OnBeginPlay(World& world, entt::entity owner);

		void OnTick(World& world, entt::entity owner, float dt);

		void OnFixedTick(World& world, entt::entity owner);

		uint32 mNumOfConstructs{};
		uint32 mNumOfBeginPlays{};
		uint32 mNumOfTicks{};
		uint32 mNumOfFixedTicks{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(EventTestingComponent);
	};
}
#endif // EDITOR