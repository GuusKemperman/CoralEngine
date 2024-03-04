#pragma once
#include "Systems/System.h"

namespace Engine
{
	class NavigationSystem final
		: public System
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
			traits.mFixedTickInterval = static_cast<float>(2);
			traits.mShouldTickBeforeBeginPlay = true;
			traits.mShouldTickWhilstPaused = true;
			return traits;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavigationSystem);
	};
}
