#ifdef EDITOR
#pragma once
#include "GSON/GSONBinary.h"

namespace CE
{
	class World;
	class TransformComponent;
	class Registry;
	class World;
	class CameraComponent;
	class FrameBuffer;

	/*
	A little helper class that manages the BeginPlay / EndPlay logic for you, and nicely
	displays the viewport, details window, and world hierarchy.

	You only need to use this class if you plan to go back and forth between BeginPlay
	and EndPlay.
	*/
	class WorldInspectHelper final
	{
	public:
		/*
		The provided doUndoStack must remain alive for the duration of this
		objects lifetime.
		*/
		WorldInspectHelper(World&& worldThatHasNotYetBegunPlay);
		~WorldInspectHelper();

		/*
		Returns the world. Note that after calling BeginPlay,
		a different world is returned up until you call EndPlay again.

		Do not call BeginPlay/EndPlay yourself, use WorldEditorSystem::BeginPlay/WorldEditorSystem::EndPlay
		*/
		World& GetWorld();

		World& GetWorldBeforeBeginPlay() { return *mWorldBeforeBeginPlay; }
		const World& GetWorldBeforeBeginPlay() const { return *mWorldBeforeBeginPlay; }

		/*
		Returns a reference to the new active world, it is only marked nodiscard to remind you
		that a different world is returned after BeginPlay/EndPlay.
		*/
		[[nodiscard]] World& BeginPlay();
		[[nodiscard]] World& EndPlay();

		/*
		By default, displays the world using the WorldViewport, WorldDetails and WorldHierarchy
		along with some BeginPlay/EndPlay buttons.
		*/
		void DisplayAndTick(float deltaTime);

		void SaveState(BinaryGSONObject& state);
		void LoadState(const BinaryGSONObject& state);

		void SwitchToFlyCam();
		void SwitchToPlayCam();

		// Can be passed to WorldViewport::Display/WorldHierarchy::Display/WorldDetails::Display, etc.
		std::vector<entt::entity> mSelectedEntities{};

		std::unique_ptr<FrameBuffer> mViewportFrameBuffer{};

	private:
		void SaveFlyCam();

		void SpawnFlyCam();
		void DestroyFlyCam();

		// Never nullptr
		std::unique_ptr<World> mWorldBeforeBeginPlay{};
		std::unique_ptr<World> mWorldAfterBeginPlay{};

		glm::mat4 mFlyCamWorldMatrix{};
		std::optional<entt::entity> mSelectedCameraBeforeWeSwitchedToFlyCam{};

		float mHierarchyHeight = .5f;
		float mDetailsHeight = .5f;

		float mViewportWidth = .75f;
		float mHierarchyAndDetailsWidth = .25f;

		static constexpr float sRunningAveragePreservePercentage = .95f;
		float mDeltaTimeRunningAverage = 1.0f / 60.0f;
	};

	class WorldViewportPanel
	{
	public:
		WorldViewportPanel() = delete;

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

	private:
		static entt::entity GetEntityThatMouseIsHoveringOver(const World& world);

		static void ShowComponentGizmos(World& world, const std::vector<entt::entity>& selectedEntities);
		static void SetGizmoRect(glm::vec2 windowPos, glm::vec2 windowSize);

		static bool GizmoManipulateSelectedTransforms(World& world,
			const std::vector<entt::entity>& selectedEntities,
			const CameraComponent& camera);
	};

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

	class WorldHierarchy
	{
	public:
		WorldHierarchy() = delete;

		/*
		Display the world hierarchy, a small menu where you can select/deselect entities and reparent them.

		doUndoStack may be nullptr. If DoUndo stack is nullptr, actions
		may still be applied to the world, but the user won't be able to undo them.

		selectedEntities may be nullptr. If selectedEntities is nullptr, this widget will behave as if no entities are selected, and
		no entities can be selected.
		*/
		static void Display(World& world,
			std::vector<entt::entity>* selectedEntities);

	private:
		// Nullopt to unparent them
		static void ReceiveDragDropOntoParent(Registry& registry,
			std::optional<entt::entity> parentAllToThisEntity);

		static void DisplayRightClickPopUp(World& world, std::vector<entt::entity>& selectedEntities);
	};
}
#endif // EDITOR