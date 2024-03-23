#pragma once

namespace Engine
{
	class PolygonColliderComponent;
}

namespace Engine
{
	class TransformComponent;
}

namespace Engine
{
	class World;
	class DiskColliderComponent;

	// Everywhere where we made an assumption of there only being X number of different collider types,
	// we place a static_assert. We don't expect to add many more collider types, but it'd be nice for
	// the person implementing a new type to know WHERE they need to add support for it.
	static constexpr uint32 sNumOfDifferentColliderTypes = 2;
	static_assert(sNumOfDifferentColliderTypes == 2, 
		"This is one more place you need to account for the new collider type."
		"Increment the number after ==, after you finish you accounting for the new collider.");


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
