#pragma once
#include "CharacterComponent.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;

	class ActiveAbilityComponent
	{
	public:
		entt::entity mCastByEntity{};

		// We need to store a copy of the character component of the character that cast the ability
		// to use if the character dies.
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