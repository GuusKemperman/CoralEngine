#pragma once

namespace Engine
{
	class World;

	/// <summary>
	/// Interface that platform specific implementations need to implement for storing world
	/// unique information on the GPU, that is passed to the renderer to render the world.
	/// </summary>
	class IGPUWorld
	{
	public:
		virtual void Update(const World& world) = 0;
	};
}