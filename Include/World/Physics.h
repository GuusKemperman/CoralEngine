#pragma once

namespace Engine
{
	class World;
	class DiskColliderComponent;
	class PolygonColliderComponent;

	/**
	 * \brief Stores the physics-related data to allow for faster queries.
	 */
	class Physics
	{
	public:
		Physics(World& world);

		Physics(Physics&&) = delete;
		Physics(const Physics&) = delete;

		Physics& operator=(Physics&&) = delete;
		Physics& operator=(const Physics&) = delete;

		/**
		 * \brief Evaluates the WorldStatic layer to determine what the height is at a given position.
		 * \return A value ranging between 0.0f and infinity.
		 */
		float GetHeightAtPosition(glm::vec2 position2D) const;

		/**
		 * \brief The max height difference that kinematic/dynamic objects are able to traverse
		 *
		 * If a physics object wants to traverse from point A to B, the height at point A and point B
		 * are compared. If it's smaller than sMaxTraversableHeightDifference, the movement is allowed.
		 * Otherwise, the object stays at point B.
		 */
		static constexpr float sMaxTraversableHeightDifference = 1.0f;

		static bool IsHeightDifferenceTraversable(float heightDiff) { return fabsf(heightDiff) <= sMaxTraversableHeightDifference; }

	private:
		// mWorld needs to be updated in World::World(World&&), so we give access to World to do so.
		friend class World;
		std::reference_wrapper<World> mWorld;

		static bool IsPointInsideCollider(glm::vec2 point, glm::vec2 colliderPos, const DiskColliderComponent& diskCollider);
		static bool IsPointInsideCollider(glm::vec2 point, glm::vec2 worldPos, const PolygonColliderComponent& polygonCollider);

		template<typename ColliderType>
		void GetHeightAtPosition(glm::vec2 position, float& highestHeight) const;
	};
}
