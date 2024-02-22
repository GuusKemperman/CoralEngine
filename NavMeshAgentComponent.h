#pragma once

namespace Engine
{
	class NavMeshAgentComponent
	{
	public:
		/**
		 * \brief Constructor to initialize the agent component
		 * \param walkingSpeed The agent's speed
		 */
		explicit NavMeshAgentComponent(float walkingSpeed);

		/**
		 * \brief Getter to grab the speed of the NavMeshAgent
		 * \return The speed of the agent as a float
		 */
		[[nodiscard]] float GetSpeed() const;

		/// \brief The quickest path from the NavMeshAgent to the KeyboardControl component
		std::vector<glm::vec2> PathFound = {};

	private:
		float Speed = 0;
	};
}
