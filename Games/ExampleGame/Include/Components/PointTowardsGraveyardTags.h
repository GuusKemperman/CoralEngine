#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	class World;
}

namespace Game
{
	class IsSignPostTag
	{
	public:
		static void OnBeginPlay(CE::World& world, entt::entity signPost);

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(IsSignPostTag);
	};

	class IsGraveyardTag
	{
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(IsGraveyardTag);
	};
}
