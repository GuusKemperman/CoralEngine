#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class TransformComponent;

	class NavMeshAgentComponent
	{
	public:

		[[nodiscard]] std::optional<glm::vec2> GetTargetPosition() const;

		void SetTargetPosition(glm::vec2 targetPosition);
		void SetTargetPosition(const TransformComponent& transformComponent);

		void UpdateTargetPosition(glm::vec2 targetPosition);
		void UpdateTargetPosition(const TransformComponent& transformComponent);

		void StopNavMesh();

		bool IsChasing() const;

		/// \brief The quickest path from the NavMeshAgent to the KeyboardControl component
		std::vector<glm::vec2> mPathFound = {};

		bool WasJustStopped();

	private:
		bool mJustStopped = false;

		std::optional<glm::vec2> mTargetPosition{};
		bool mIsChasing = true;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavMeshAgentComponent);
	};
}
