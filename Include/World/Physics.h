#pragma once
#include "Components/TransformComponent.h"
#include "Meta/MetaReflect.h"

namespace CE
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

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Physics);

		// mWorld needs to be updated in World::World(World&&), so we give access to World to do so.
		friend class World;
		std::reference_wrapper<World> mWorld;
	};
}
