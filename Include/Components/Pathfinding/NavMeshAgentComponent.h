#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class NavMeshAgentComponent
	{
	public:
		/**
		 * \brief Getter to grab the speed of the NavMeshAgent
		 * \return The speed of the agent as a float
		 */
		[[nodiscard]] float GetSpeed() const;

		float mSpeed = 0;

		/// \brief The quickest path from the NavMeshAgent to the KeyboardControl component
		std::vector<glm::vec2> mPathFound = {};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavMeshAgentComponent);
	};
}
