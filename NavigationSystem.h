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

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mPriority = static_cast<int>(TickPriorities::PostPhysics);
			traits.mShouldTickBeforeBeginPlay = true;
			traits.mShouldTickWhilstPaused = true;
			return traits;
		}

	private:
		float FixedDt = 1.0f / 10.0f;
		float FixedTimeAccumulator = 0.0f;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavigationSystem);
	};
}
