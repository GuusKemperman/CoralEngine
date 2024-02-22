#pragma once
#include "NavMeshComponent.h"
#include "Systems/System.h"

namespace Engine
{
	class NavigationSystem final : public System
	{
	public:
		/**
		 * \brief Updates the navigation system
		 * \param world the current world
		 * \param dt Delta time
		 */
		void Update(World& world, float dt) override;

		/**
		 * \brief Renders the navigation system
		 * \param world the current world
		 */
		void Render(const World& world) override;

	private:
		float FixedDt = 1.0f / 10.0f;
		float FixedTimeAccumulator = 0.0f;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavigationSystem);
	};
}
