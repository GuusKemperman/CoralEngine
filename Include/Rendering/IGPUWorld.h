#pragma once

namespace CE
{
	class World;

	/// <summary>
	/// Interface that platform specific implementations need to implement for storing world
	/// unique information on the GPU, that is passed to the renderer to render the world.
	/// </summary>
	class IGPUWorld
	{
		// mWorld needs to be updated in World::World(World&&), so we give access to World to do so
		friend class World;

	public:
		IGPUWorld(const World& world)
			:
			mWorld(world)
		{}
		virtual void Update() = 0;

	protected:
		std::reference_wrapper<const World> mWorld;
	};
}