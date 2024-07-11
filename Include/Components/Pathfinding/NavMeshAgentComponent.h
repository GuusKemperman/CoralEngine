#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;

	class NavMeshAgentComponent
	{
	public:
		void OnConstruct(World& world, entt::entity owner);

		std::optional<glm::vec2> GetTargetPosition(const World& world) const;
		entt::entity GetTargetEntity() const;

		void SetTargetPosition(glm::vec2 targetPosition);
		void SetTargetEntity(entt::entity entity);

		void ClearTarget(World& world);

		bool IsChasing() const;

		std::vector<glm::vec2> mPath{};

		using TargetT = std::variant<std::monostate, glm::vec2, entt::entity>;
		TargetT mTarget{};

		float mAvoidanceDistance = 5.0f;

		float mAdditionalDistanceAlongPathToTarget = 25.0f;

	private:
		entt::entity mOwner{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavMeshAgentComponent);
	};
}
