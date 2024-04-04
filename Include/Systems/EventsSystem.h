#pragma once
#include "Systems/System.h"
#include "Utilities/Events.h"

namespace CE
{
	class TickSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(TickSystem);
	};

	class FixedTickSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mFixedTickInterval = sFixedTickEventStepSize;
			return traits;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(FixedTickSystem);
	};
}
