#pragma once
#include "Meta/MetaReflect.h"
#include "Utilities/Events.h"

namespace CE
{
	struct OnButtonPressed :
		EventType<OnButtonPressed>
	{
		OnButtonPressed() :
			EventType("OnButtonPressed")
		{
		}
	};
	/**
	 * \brief Called when the button is pressed. Must be attached to the entity with the UIButtonTag.
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 */
	inline const OnButtonPressed sOnButtonPressed{};

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
