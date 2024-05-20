#pragma once
#ifdef EDITOR
#include <imgui/ImGuizmo.h>

namespace CE
{
	class World;
	class FrameBuffer;

	class WorldViewportPanel
	{
	public:
		/*
		Renders the world to an ImGui::Image and allows the user to interact with it through ImGuizmo and drag drop.

		The world is rendered to the framebuffer. The framebuffer will be resized if needed, a default constructed
		framebuffer works just fine. The framebuffer should be kept alive across frames, don't create a framebuffer
		on the stack and expect it to work.

		doUndoStack may be nullptr. If DoUndo stack is nullptr, actions
		may still be applied to the world, but the user won't be able to undo them.

		selectedEntities may be nullptr. If selectedEntities is nullptr, this widget will behave as if no entities are selected, and
		no entities can be selected.
		*/
		static void Display(World& world, FrameBuffer& frameBuffer,
			std::vector<entt::entity>* selectedEntities);
	};

	namespace Internal
	{
		inline ImGuizmo::OPERATION sGuizmoOperation{ ImGuizmo::OPERATION::TRANSLATE };
		inline ImGuizmo::MODE sGuizmoMode{ ImGuizmo::MODE::WORLD };
		inline bool sShouldGuizmoSnap{};

		// Different variable for each operation.
		inline float sTranslateSnapTo{ 1.0f };
		inline float sRotationSnapTo{ 30.0f };
		inline float sScaleSnapTo{ 1.0f };

		// Storing it so that we only have to do the check once.
		inline float* sCurrentSnapTo{};

		// We need this for the Manipulate() function
		// which expects a vec3 in the form of a pointer to float.
		inline glm::vec3 sCurrentSnapToVec3{};
	}
}
#endif // EDITOR