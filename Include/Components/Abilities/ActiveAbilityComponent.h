#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class ActiveAbilityComponent
	{
	public:
		entt::entity mCastByCharacter{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ActiveAbilityComponent);
	};
}