#ifdef EDITOR
#pragma once

namespace CE
{
	class World;
	class FrameBuffer;
	class BinaryGSONObject;

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
		WorldInspectHelper(std::unique_ptr<World> worldThatHasNotYetBegunPlay);
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
		float mFlyCamSpeed{};
		std::optional<entt::entity> mSelectedCameraBeforeWeSwitchedToFlyCam{};

		float mHierarchyHeight = .5f;
		float mDetailsHeight = .5f;

		float mViewportWidth = .75f;
		float mHierarchyAndDetailsWidth = .25f;

		static constexpr float sRunningAveragePreservePercentage = .95f;
		float mDeltaTimeRunningAverage = 1.0f / 60.0f;
	};

	namespace Internal
	{
		void RemoveInvalidEntities(World& world, std::vector<entt::entity>& selectedEntities);
		void ReceiveDragDrops(World& world);

		bool ToggleIsEntitySelected(std::vector<entt::entity>& selectedEntities, entt::entity toSelect);

		entt::entity GetEntityThatMouseIsHoveringOver(const World& world);

		void DeleteEntities(World& world, std::vector<entt::entity>& selectedEntities);

		std::optional<std::string_view> GetSerializedEntitiesInClipboard();
		std::string CopyToClipBoard(const World& world, const std::vector<entt::entity>& selectedEntities);
		void CutToClipBoard(World& world, std::vector<entt::entity>& selectedEntities);
		void PasteClipBoard(World& world, std::vector<entt::entity>& selectedEntities, std::string_view clipboardData);
		void Duplicate(World& world, std::vector<entt::entity>& selectedEntities);

		enum ShortCutType
		{
			Delete = 1 << 1,
			CopyPaste = 1 << 2,
			SelectDeselect = 1 << 3,
			GuizmoModes = 1 << 4
		};

		void CheckShortcuts(World& world, std::vector<entt::entity>& selectedEntities, ShortCutType types);
	}
}
#endif // EDITOR