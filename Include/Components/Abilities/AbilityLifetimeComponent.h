#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	// AOE (Area Of Attack) abilities need this,
	// but it can also be utilised however the user sees fit.
	class AbilityLifetimeComponent
	{
	public:
		// How long the ability should be alive for.
		// You need this for abilities even if you want it to be instant
		// so that it stays on screen more than one frame.
		float mDuration = 0.2f;

		// How long it has been alive for.
		float mDurationTimer{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilityLifetimeComponent);
	};
}
