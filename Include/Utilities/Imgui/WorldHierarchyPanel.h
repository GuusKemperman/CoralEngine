#pragma once

#ifdef EDITOR

namespace CE
{
	class World;

	class WorldHierarchy
	{
	public:
		/*
		Display the world hierarchy, a small menu where you can select/deselect entities and reparent them.

		doUndoStack may be nullptr. If DoUndo stack is nullptr, actions
		may still be applied to the world, but the user won't be able to undo them.

		selectedEntities may be nullptr. If selectedEntities is nullptr, this widget will behave as if no entities are selected, and
		no entities can be selected.
		*/
		static void Display(World& world,
			std::vector<entt::entity>* selectedEntities);
	};
}

#endif // EDITOR