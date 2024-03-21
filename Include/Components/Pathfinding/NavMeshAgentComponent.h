#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class TransformComponent;

	class NavMeshAgentComponent
	{
	public:

		[[nodiscard]] std::optional<glm::vec2> GetTargetPosition() const;

		void SetTarget(glm::vec2 targetPosition);
		void SetTarget(const TransformComponent& transformComponent);

		void StopNavMesh();

		bool IsChasing() const;

		/// \brief The quickest path from the NavMeshAgent to the KeyboardControl component
		std::vector<glm::vec2> mPathFound = {};

	private:
		std::optional<glm::vec2> mTargetPosition{};
		bool mIsChasing = true;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavMeshAgentComponent);
	};
}
