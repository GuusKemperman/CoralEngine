#pragma once

#ifdef EDITOR

namespace CE
{
	class World;

	class WorldDetails
	{
	public:
		WorldDetails() = delete;

		/*
		Display a details window where the components of the selected entities can be inspected, added or removed.

		doUndoStack may be nullptr. If DoUndo stack is nullptr, actions
		may still be applied to the world, but the user won't be able to undo them.
		*/
		static void Display(World& world,
			std::vector<entt::entity>& selectedEntities);
	};
}

#endif // EDITOr