#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class PlayerComponent
	{
	public:
		int mID{};

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(PlayerComponent);
	};
}
