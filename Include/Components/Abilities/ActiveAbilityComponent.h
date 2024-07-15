#pragma once
#include "CharacterComponent.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;

	class ActiveAbilityComponent
	{
	public:
#ifdef EDITOR
		void OnInspect(World& world, entt::entity owner);
#endif // EDITOR

		entt::entity mCastByEntity{};

		// We need to store a copy of the character component of the character that cast the ability
		// to use if the character dies.
		CharacterComponent mCastByCharacterData{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ActiveAbilityComponent);
	};
}