#pragma once
#include "CharacterComponent.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;

	class ActiveAbilityComponent
	{
	public:
		CharacterComponent mCastByCharacterData{};

#ifdef EDITOR
		static void OnInspect(World& world, const std::vector<entt::entity>& entities);
#endif // EDITOR

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ActiveAbilityComponent);
	};
}