#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class UIButtonSelectedTag
	{
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UIButtonSelectedTag);
	};

	class World;

	class UIButtonTag
	{
	public:
		static void Select(World& world, entt::entity buttonOwner);
		static void Deselect(World& world);

		static bool IsSelected(const World& world, entt::entity buttonOwner);

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UIButtonTag);
	};
}