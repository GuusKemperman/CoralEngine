#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class TransformComponent;

	class NavMeshAgentComponent
	{
	public:
		/**
		 * \brief Getter to grab the speed of the NavMeshAgent
		 * \return The speed of the agent as a float
		 */
		[[nodiscard]] float GetSpeed() const;

		[[nodiscard]] std::optional<glm::vec2> GetTargetPosition() const;

		void SetTarget(glm::vec2 targetPosition);
		void SetTarget(const TransformComponent& transformComponent);

		void StopNavMesh();

		/// \brief The quickest path from the NavMeshAgent to the KeyboardControl component
		std::vector<glm::vec2> mPathFound = {};

	private:
		float mSpeed = 0;
		std::optional<glm::vec2> mTargetPosition{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavMeshAgentComponent);
	};
}
