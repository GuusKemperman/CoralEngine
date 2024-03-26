#pragma once

namespace Engine
{
	class World;

	class ISubRenderer
	{
	public:
		virtual ~ISubRenderer() = default;
		virtual void Render(const World& world) = 0;
	};
}