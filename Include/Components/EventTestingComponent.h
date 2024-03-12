#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class World;

	class EmptyEventTestingComponent
	{
	public:
		static void OnConstruct(World& world, entt::entity owner);

		static void OnDestruct(World& world, entt::entity owner);

		static void OnBeginPlay(World& world, entt::entity owner);

		static void OnTick(World& world, entt::entity owner, float dt);

		static void OnFixedTick(World& world, entt::entity owner);

		static void OnAiTick(World& world, entt::entity owner, float dt);

		static float OnAiEvaluate(const World& world, entt::entity owner);

		static void OnCollisionEntry(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2);
		static void OnCollisionStay(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2);
		static void OnCollisionExit(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2);

		static uint32 GetValue(Name valueName);

		static inline uint32 sNumOfConstructs{};
		static inline uint32 sNumOfDestructs{};
		static inline uint32 sNumOfBeginPlays{};
		static inline uint32 sNumOfTicks{};
		static inline uint32 sNumOfFixedTicks{};
		static inline uint32 sNumOfAiTicks{};
		static inline uint32 sNumOfAiEvaluates{};
		static inline uint32 sNumOfCollisionEntry{};
		static inline uint32 sNumOfCollisionStay{};
		static inline uint32 sNumOfCollisionExit{};

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

		void OnDestruct(World& world, entt::entity owner);

		void OnBeginPlay(World& world, entt::entity owner);

		void OnTick(World& world, entt::entity owner, float dt);

		void OnFixedTick(World& world, entt::entity owner);

		void OnAiTick(World& world, entt::entity owner, float dt);

		float OnAiEvaluate(const World& world, entt::entity owner) const;

		void OnCollisionEntry(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2);
		void OnCollisionStay(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2);
		void OnCollisionExit(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2);

		uint32 mNumOfConstructs{};
		uint32 mNumOfDestructs{};
		uint32 mNumOfBeginPlays{};
		uint32 mNumOfTicks{};
		uint32 mNumOfFixedTicks{};
		uint32 mNumOfAiTicks{};
		mutable uint32 mNumOfAiEvaluates{};
		uint32 mNumOfCollisionEntry{};
		uint32 mNumOfCollisionStay{};
		uint32 mNumOfCollisionExit{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(EventTestingComponent);
	};
}
