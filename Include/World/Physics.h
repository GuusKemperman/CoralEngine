#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class World;
	class TransformComponent;

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
		 * \return A value ranging between -infinity (if there is no terrain) and infinity (if the terrain is reaaallyy high).
		 */
		float GetHeightAtPosition(glm::vec2 position2D) const;

		/**
		 * \brief Moves the transform to the requested position while preserving the height difference relative to the terrain.
		 */
		void Teleport(TransformComponent& transform, glm::vec2 toPosition) const;

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
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Physics);

		// mWorld needs to be updated in World::World(World&&), so we give access to World to do so.
		friend class World;
		std::reference_wrapper<World> mWorld;

		template<typename ColliderType>
		void GetHeightAtPosition(glm::vec2 position, float& highestHeight) const;
	};
}
