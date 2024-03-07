#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class ActiveAbilityComponent
	{
	public:
		entt::entity mCastByPlayer{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ActiveAbilityComponent);
	};
}