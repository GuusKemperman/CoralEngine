#ifdef EDITOR
#pragma once
#include "EditorSystems/AssetEditorSystems/AssetEditorSystem.h"

#include "imnodes/imgui_node_editor.h"
#include "Assets/Script.h"

namespace ax::NodeEditor::Utilities
{
	struct BlueprintNodeBuilder;
}

namespace Engine
{
	class RerouteScriptNode;
	class CommentScriptNode;
	class ScriptFunc;
	class ScriptField;

	class ScriptEditorSystem final :
		public AssetEditorSystem<Script>
	{
	public:
		ScriptEditorSystem(Script&& asset);
		~ScriptEditorSystem() override;

		ScriptEditorSystem(const ScriptEditorSystem&) = delete;
		ScriptEditorSystem& operator=(const ScriptEditorSystem&) = delete;

		void Tick(float deltaTime) override;

		void SaveState(std::ostream& toStream) const override;
		void LoadState(std::istream& fromStream) override;

		void NavigateTo(const ScriptLocation& location);

	private:
		const ScriptFunc* TryGetSelectedFunc() const;
		ScriptFunc* TryGetSelectedFunc();

		const ScriptField* TryGetSelectedField() const;
		ScriptField* TryGetSelectedField();

		void DeselectCurrentFieldOrFunc();

		void SelectFunction(ScriptFunc* func);
		void SelectField(ScriptField* field);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ScriptEditorSystem);

		std::vector<NodeId> GetSelectedNodes() const;
		std::vector<LinkId> GetSelectedLinks() const;

		void ReadInput();
		void DeleteSelection();
		void CopySelection();
		void CutSelection();
		void DuplicateSelection();

		// If no offset is provided, the nodes
		// are pasted at the position of the cursor.
		void Paste(std::optional<glm::vec2> offsetToOldPos = std::nullopt);

		void SaveFunctionState() const;
		void LoadFunctionState();

		static constexpr std::array<float, 6> sZoomLevels
		{
			.03f,
			.06f,
			.125f,
			.25f,
			.5f,
			1.0f,
		};

		float mOverviewPanelWidth = .17f;
		float mCanvasWidth = .8f;
		float mDetailsWidth = .17f;

		// Starts to be slightly different than mCanvasWidth + mDetailsWidth
		// due to some imgui padding, which is why we store it seperately as well
		float mCanvasPlusDetailsWidth = mCanvasWidth + mDetailsWidth;

		ax::NodeEditor::EditorContext* mContext{};

		uint32 mIndexOfCurrentFunc = std::numeric_limits<uint32>::max();
		uint32 mIndexOfCurrentField = std::numeric_limits<uint32>::max();

		// The key is the name of the function.
		mutable std::unordered_map<std::string, glm::vec4> mCanvasRect{};

		//************************************************//
		//		Defined in ScriptDetailsPanel.cpp		  //
		//************************************************//

		void DisplayDetailsPanel();

		void DisplayFunctionDetails(ScriptFunc& func);
		void DisplayNodeDetails(ScriptNode& node);
		void DisplayMemberDetails(ScriptField& field);

		friend struct ParamWrapper;

		//************************************************//
		//		Defined in ScriptCanvasPanel.cpp		  //
		//************************************************//

		struct NodeTheUserCanAdd
		{
			NodeTheUserCanAdd(std::string&& category, 
				std::string&& name, 
				std::function<ScriptNode&(ScriptFunc&)>&& addNode,
				std::function<bool(const ScriptPin&)>&& matchesContext) :
			mCategory(std::move(category)),
			mName(std::move(name)),
			mAddNode(std::move(addNode)),
			mMatchesContext(std::move(matchesContext)) {}

			std::string mCategory{};
			std::string mName{};
			std::function<ScriptNode& (ScriptFunc&)> mAddNode{};
			std::function<bool(const ScriptPin&)> mMatchesContext{};

			// How similar the name of this item is to the string the user is searching for
			double mSimilarityToQuery = 100.0;
		};

		void DisplayCanvas();
		void DrawCanvasObjects();
		void TryLetUserCreateLink();
		 
		void DeleteRequestedItems();

		void DisplayCanvasPopUps();
		void DisplayCreateNewNowPopUp(ImVec2 placeNodeAtPos);
		void DisplayPinContextPopUp();

		struct PinToInspect
		{
			PinId mPinId{};
			ImVec2 mPosition{};
		};

		void DisplayFunctionNode(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder, ScriptNode& node, std::vector<PinToInspect>& pinsToInspect);
		void DisplayCommentNode(CommentScriptNode& node);

		bool IsRerouteNodeFlipped(const RerouteScriptNode& node) const;
		void DisplayRerouteNode(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder, RerouteScriptNode& node);

		bool CanAnyPinsBeInspected() const;

		void DisplayPin(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder, 
			ScriptPin& pin, 
			std::vector<PinToInspect>& pinsToInspect);

		bool ShouldWeOnlyShowContextMatchingNodes() const;
		static bool DoesNodeMatchContext(const ScriptPin& toPin,
			TypeTraits returnTypeTraits, 
			const std::vector<TypeTraits>& parameters, 
			bool isPure);
		bool DoesNodeMatchContext(const NodeTheUserCanAdd& node) const;
		static inline bool sContextSensitive = true;

		bool ShouldShowUserNode(const NodeTheUserCanAdd& node) const;

		void UpdateSimilarityToQuery();
		void ClearQuery();

		ImColor GetIconColor(const ScriptVariableTypeData& typeData) const;
		void DrawPinIcon(const ScriptPin& pin, bool connected, int alpha, bool mirrored = false) const;

		std::vector<NodeTheUserCanAdd> GetALlNodesTheUserCanAdd() const;

		std::vector<NodeTheUserCanAdd> mNodesThatCanBeCreated;

		std::vector<std::reference_wrapper<NodeTheUserCanAdd>> mRecommendedNodesBasedOnQuery{};
		std::string mCurrentQuery{};

		// This makes it more likely to show functions and fields from
		// this script when searching.
		static constexpr double sBiasTowardsNodesFromThisScript = 1.5f;
		static constexpr double sSimilarityCuttOff = 50.0f;
		static constexpr uint32 sMaxNumOfRecommendedNodesDuringQuery = 5;

		ax::NodeEditor::PinId mPinTheUserRightClicked{};
		ax::NodeEditor::PinId mPinTheUserIsTryingToLink{};
		std::optional<ImVec2> mCreateNodePopUpPosition{};

		// After switching to a different function,
		// we cannot immediately centre the camera on
		// a single node, since the node editor has not
		// yet received all the information about the location
		// and size of each node. This is why after navigating
		// to a new function, we only start navigating to
		// a specific node at the end of the frame, after
		// everything has been drawn.
		//
		// The int32 represents the number of frames since
		// the request has been made. We only navigate after
		// a few frames, due to a bug in the scripting library
		// we are using.
		std::pair<int32, ScriptLocation> mNavigateToLocationAtEndOfFrame{ -1, ScriptLocation{ mAsset } };

		static constexpr int sPinIconSize = 28;
		static constexpr glm::vec2 sMinInspectPinWindowSize{ static_cast<float>(sPinIconSize) };

		//************************************************//
		//		Defined in ScriptClassPanel.cpp 		  //
		//************************************************//
		void DisplayClassPanel();

		void DisplayEventsOverview();
		void DisplayFunctionsOverview();
		void DisplayMembersOverview();

		struct OverviewResult
		{
			bool mAddButtonPressed{};
			std::optional<Name> mDeleteItemWithThisName{};
			std::optional<Name> mSwitchedToName{};
		};

		// The parts of DisplayFunctionsOverview and DisplayMembersOverview that were similar enough
		// to be extracted to it's own function, to reduce code repetition.
		static OverviewResult DisplayOverview(const char* label, std::vector<Name> names, std::optional<Name> nameOfCurrentlySelected);
	};
}
#endif // EDITOR