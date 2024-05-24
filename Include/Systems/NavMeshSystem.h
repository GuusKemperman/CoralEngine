#pragma once
#include "Systems/System.h"

namespace CE
{
	class NavigationSystem final
		: public System
	{
	public:
		void Update(World& world, float dt) override;

		void Render(const World& world) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mPriority = static_cast<int>(TickPriorities::PreTick);
			return traits;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavigationSystem);
	};

	class UpdatePathsSystem final
		: public System
	{
	public:
		void Update(World& world, float dt) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mPriority = static_cast<int>(TickPriorities::PreTick);
			traits.mFixedTickInterval = .2f;
			return traits;
		}

	private:
		static constexpr uint32 sUpdateEveryNthPath = 10;
		uint32 mNumOfTicksReceived{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UpdatePathsSystem);
	};
}
