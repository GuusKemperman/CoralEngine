#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class UIButtonSelectedTag
	{
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UIButtonSelectedTag);
	};

	class World;

	class UIButtonComponent
	{
	public:
		entt::entity mButtonTopSide{ entt::null };
		entt::entity mButtonBottomSide{ entt::null };
		entt::entity mButtonRightSide{ entt::null };
		entt::entity mButtonLeftSide{ entt::null };

		static void Select(World& world, entt::entity buttonOwner);
		static void Deselect(World& world);

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UIButtonComponent);
	};
}