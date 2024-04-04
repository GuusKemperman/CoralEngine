#include "Precomp.h"
#include "EditorSystems/AssetEditorSystems/ScriptEditorSystem.h"

#include <imgui/imgui_internal.h>

#include "Core/Input.h"
#include "Core/VirtualMachine.h"
#include "GSON/GSONBinary.h"
#include "Scripting/ScriptNode.h"
#include "Utilities/ClassVersion.h"
#include "Utilities/Imgui/ImguiHelpers.h"
#include "Utilities/StringFunctions.h"

CE::ScriptEditorSystem::ScriptEditorSystem(Script&& asset) :
	AssetEditorSystem(std::move(asset))
{
	mLastUpToDateQueryData.mNodesThatCanBeCreated = GetAllNodesTheUserCanAdd();
	mAsset.PostDeclarationRefresh();
}

CE::ScriptEditorSystem::~ScriptEditorSystem()
{
	if (mQueryThread.joinable())
	{
		mQueryThread.join();
	}
	SelectFunction(nullptr);
}

void CE::ScriptEditorSystem::Tick(const float deltaTime)
{
	if (!Begin(ImGuiWindowFlags_MenuBar))
	{
		End();
		return;
	}

	AssetEditorSystem::Tick(deltaTime);
	ax::NodeEditor::SetCurrentEditor(mContext);

	if (ImGui::BeginMenuBar())
	{
		ShowSaveButton();

		ImGui::BeginDisabled(mContext == nullptr);

		if (ImGui::Button(ICON_FA_SEARCH_PLUS))
		{
			ax::NodeEditor::NavigateToContent();
		}
		ImGui::SetItemTooltip("Zoom to fit function contents");

		ImGui::EndDisabled();

		ImGui::EndMenuBar();
	}

	ImGui::Splitter(true, &mOverviewPanelWidth, &mCanvasPlusDetailsWidth);

	if (ImGui::BeginChild("OverviewPanel", { mOverviewPanelWidth, 0.0f }))
	{
		DisplayClassPanel();
	}

	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::PushID(2); // Second splitter requires new ID
	ImGui::Splitter(true, &mCanvasWidth, &mDetailsWidth);
	ImGui::PopID();

	if (ImGui::BeginChild("Canvas", { mCanvasWidth, 0.0f }))
	{
		DisplayCanvas();
	}
	ImGui::EndChild();

	ImGui::SameLine();
	if (ImGui::BeginChild("Details", { mDetailsWidth, 0.0f }))
	{
		DisplayDetailsPanel();
	}
	ImGui::EndChild();

	ReadInput();

	if (mNavigateToLocationAtEndOfFrame.first != -1)
	{
		// Bug in ax::nodeeditor library that requires us to wait a few frames
		if (mNavigateToLocationAtEndOfFrame.first++ >= 2)
		{
			ASSERT(TryGetSelectedFunc() != nullptr && "We reset mNavigateToLocationAtEndOfFrame when deselecting anything");
			ASSERT(TryGetSelectedFunc()->GetName() == mNavigateToLocationAtEndOfFrame.second.mFieldOrFuncName);

			const ScriptFunc& func = *TryGetSelectedFunc();

			switch (mNavigateToLocationAtEndOfFrame.second.mType)
			{
			case ScriptLocation::Type::Script:
			case ScriptLocation::Type::Field:
				break;
			case ScriptLocation::Type::Node:
			{
				NodeId nodeId = std::get<NodeId>(mNavigateToLocationAtEndOfFrame.second.mId);

				if (func.TryGetNode(nodeId) == nullptr)
				{
					LOG(LogEditor, Message, "Cannot navigate to node, node no longer exists");
				}
				else
				{
					ax::NodeEditor::SelectNode(nodeId);
					ax::NodeEditor::NavigateToSelection();
				}

				break;
			}
			case ScriptLocation::Type::Pin:
			{
				const auto& [pinId, nodeId] = std::get<std::pair<PinId, NodeId>>(mNavigateToLocationAtEndOfFrame.second.mId);

				if (func.TryGetPin(pinId) == nullptr)
				{
					LOG(LogEditor, Message, "Cannot navigate to node, node no longer exists");
				}
				else
				{
					ax::NodeEditor::SelectNode(nodeId);
					ax::NodeEditor::NavigateToSelection();
				}

				break;
			}
			case ScriptLocation::Type::Link:
			{
				LinkId linkId = std::get<LinkId>(mNavigateToLocationAtEndOfFrame.second.mId);

				if (func.TryGetLink(linkId) == nullptr)
				{
					LOG(LogEditor, Message, "Cannot navigate to link, link no longer exists");
				}
				else
				{
					ax::NodeEditor::SelectLink(linkId);
					ax::NodeEditor::NavigateToSelection();
				}

				break;
			}
			case ScriptLocation::Type::Func:
			{
				ax::NodeEditor::NavigateToContent();
				break;
			}
			}

			mNavigateToLocationAtEndOfFrame.first = -1;
		}
	}

	ax::NodeEditor::SetCurrentEditor(nullptr);
	End();
}

void CE::ScriptEditorSystem::SelectFunction(ScriptFunc* func)
{
	if (TryGetSelectedFunc() == func)
	{
		return;
	}

	SaveFunctionState();
	DestroyEditor(mContext);
	mContext = nullptr;
	mIndexOfCurrentFunc = std::numeric_limits<uint32>::max();
	mNavigateToLocationAtEndOfFrame.first = -1;
	ax::NodeEditor::SetCurrentEditor(nullptr);

	if (func == nullptr)
	{
		return;
	}

	for (uint32 i = 0; i < mAsset.GetFunctions().size(); i++)
	{
		if (&mAsset.GetFunctions()[i] == func)
		{
			mIndexOfCurrentFunc = i;
			break;
		}
	}
	ASSERT(TryGetSelectedFunc() == func);

	ax::NodeEditor::Config config{};
	config.SettingsFile = "";

	for (float zoomLevel : sZoomLevels)
	{
		config.CustomZoomLevels.push_back(zoomLevel);
	}

	mContext = CreateEditor(&config);
	SetCurrentEditor(mContext);
	ax::NodeEditor::EnableShortcuts(false);

	LoadFunctionState();
}

void CE::ScriptEditorSystem::SelectField(ScriptField* field)
{
	if (TryGetSelectedField() == field)
	{
		return;
	}

	mIndexOfCurrentField = std::numeric_limits<uint32>::max();

	for (uint32 i = 0; i < mAsset.GetFields().size(); i++)
	{
		if (&mAsset.GetFields()[i] == field)
		{
			mIndexOfCurrentField = i;
			break;
		}
	}
	ASSERT(TryGetSelectedField() == field);
}

void CE::ScriptEditorSystem::SaveState(std::ostream& toStream) const
{
	AssetEditorSystem::SaveState(toStream);

	SaveFunctionState();

	BinaryGSONObject object{};

	object.AddGSONMember("rects") << mCanvasRect;

	const ScriptFunc* const selectedFunc = TryGetSelectedFunc();
	const ScriptField* const selectedMember = TryGetSelectedField();

	if (selectedFunc != nullptr)
	{
		object.AddGSONMember("selectedFunc") << selectedFunc->GetName();
	}
	else if (selectedMember != nullptr)
	{
		object.AddGSONMember("selectedMember") << selectedMember->GetName();
	}

	object.SaveToBinary(toStream);
}

void CE::ScriptEditorSystem::LoadState(std::istream& fromStream)
{
	AssetEditorSystem::LoadState(fromStream);

	BinaryGSONObject object{};
	if (!object.LoadFromBinary(fromStream))
	{
		return;
	}

	const BinaryGSONMember* selectedFunc = object.TryGetGSONMember("selectedFunc");
	const BinaryGSONMember* selectedMember = object.TryGetGSONMember("selectedMember");
	const BinaryGSONMember* rects = object.TryGetGSONMember("rects");

	if (rects != nullptr)
	{
		*rects >> mCanvasRect;
	}

	if (selectedFunc != nullptr)
	{
		std::string name{};
		*selectedFunc >> name;
		SelectFunction(mAsset.TryGetFunc(Name{ name }));
	}
	else if (selectedMember != nullptr)
	{
		std::string name{};
		*selectedMember >> name;
		SelectField(mAsset.TryGetField(Name{ name }));
	}
}

void CE::ScriptEditorSystem::NavigateTo(const ScriptLocation& location)
{
	if (location.mNameOfScript != mAsset.GetName())
	{
		LOG(LogEditor, Error, "Cannot navigate to location {}, as this editor is editing a different script ({})",
			location.ToString(), mAsset.GetName());
		return;
	}

	if (location.mType == ScriptLocation::Type::Field)
	{
		ScriptField* field = mAsset.TryGetField(location.mFieldOrFuncName);

		if (field == nullptr)
		{
			LOG(LogEditor, Message, "Cannot navigate to location {}, field {} no longer exists", location.ToString(), location.mFieldOrFuncName);
			return;
		}

		SelectField(field);
	}
	else if (location.mType != ScriptLocation::Type::Script)
	{
		ScriptFunc* func = mAsset.TryGetFunc(location.mFieldOrFuncName);

		if (func == nullptr)
		{
			LOG(LogEditor, Message, "Cannot navigate to location {}, function {} no longer exists", location.ToString(), location.mFieldOrFuncName);
			return;
		}

		SelectFunction(func);
		mNavigateToLocationAtEndOfFrame = { 0, location };
	}
}

const CE::ScriptFunc* CE::ScriptEditorSystem::TryGetSelectedFunc() const
{
	return mIndexOfCurrentFunc < mAsset.GetFunctions().size() ? &mAsset.GetFunctions()[mIndexOfCurrentFunc] : nullptr;
}

CE::ScriptFunc* CE::ScriptEditorSystem::TryGetSelectedFunc()
{
	return const_cast<ScriptFunc*>(const_cast<const ScriptEditorSystem&>(*this).TryGetSelectedFunc());
}

const CE::ScriptField* CE::ScriptEditorSystem::TryGetSelectedField() const
{
	return mIndexOfCurrentField < mAsset.GetFields().size() ? &mAsset.GetFields()[mIndexOfCurrentField] : nullptr;
}

CE::ScriptField* CE::ScriptEditorSystem::TryGetSelectedField()
{
	return const_cast<ScriptField*>(const_cast<const ScriptEditorSystem&>(*this).TryGetSelectedField());
}

CE::MetaType CE::ScriptEditorSystem::Reflect()
{
	return { MetaType::T<ScriptEditorSystem>{}, "ScriptEditorSystem",
		MetaType::Base<AssetEditorSystem<Script>>{},
		MetaType::Ctor<Script&&>{} };
}

std::vector<CE::NodeId> CE::ScriptEditorSystem::GetSelectedNodes() const
{
	std::vector<ax::NodeEditor::NodeId> selectedNodes{};
	selectedNodes.resize(ax::NodeEditor::GetSelectedObjectCount());
	const int nodeCount = ax::NodeEditor::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
	selectedNodes.resize(nodeCount);
	return { selectedNodes.data(), selectedNodes.data() + selectedNodes.size() };
}

std::vector<CE::LinkId> CE::ScriptEditorSystem::GetSelectedLinks() const
{
	std::vector<ax::NodeEditor::LinkId> selectedLinks{};
	selectedLinks.resize(ax::NodeEditor::GetSelectedObjectCount());
	const int nodeCount = ax::NodeEditor::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));
	selectedLinks.resize(nodeCount);
	return { selectedLinks.data(), selectedLinks.data() + selectedLinks.size() };
}

void CE::ScriptEditorSystem::ReadInput()
{
	if (TryGetSelectedFunc() == nullptr)
	{
		return;
	}

	const Input& input = Input::Get();

	if (input.IsKeyboardKeyHeld(Input::KeyboardKey::LeftControl) || input.IsKeyboardKeyHeld(Input::KeyboardKey::RightControl))
	{
		if (input.WasKeyboardKeyPressed(Input::KeyboardKey::C))
		{
			CopySelection();
		}
		else if (input.WasKeyboardKeyPressed(Input::KeyboardKey::V))
		{
			Paste();
		}
		else if (input.WasKeyboardKeyPressed(Input::KeyboardKey::D))
		{
			DuplicateSelection();
		}
		else if (input.WasKeyboardKeyPressed(Input::KeyboardKey::X))
		{
			CutSelection();
		}

		for (const NodeTheUserCanAdd& node : mLastUpToDateQueryData.mNodesThatCanBeCreated)
		{
			if (node.mShortCut.has_value()
				&& input.WasKeyboardKeyPressed(*node.mShortCut))
			{
				AddNewNode(node);
			}
		}

	}

	if (input.WasKeyboardKeyPressed(Input::KeyboardKey::Delete))
	{
		DeleteSelection();
	}
}

void CE::ScriptEditorSystem::DeleteSelection()
{
	std::vector<NodeId> nodes = GetSelectedNodes();
	std::vector<LinkId> links = GetSelectedLinks();

	ax::NodeEditor::BeginDelete();

	for (NodeId node : nodes)
	{
		ax::NodeEditor::DeleteNode(node);
	}

	for (LinkId link : links)
	{
		ax::NodeEditor::DeleteLink(link);
	}

	ax::NodeEditor::EndDelete();
}

static constexpr std::string_view sClipboardScriptIdentifier = "A0B1ZZ";

void CE::ScriptEditorSystem::CopySelection()
{
	const ScriptFunc* func = TryGetSelectedFunc();

	if (func == nullptr)
	{
		return;
	}

	std::vector<NodeId> nodes = GetSelectedNodes();

	BinaryGSONObject object{};
	BinaryGSONObject& nodesObject = object.AddGSONObject("nodes");

	for (NodeId nodeId : nodes)
	{
		const ScriptNode* node = func->TryGetNode(nodeId);

		if (node == nullptr)
		{
			LOG(LogEditor, Warning, "Could not copy node, node does not exist anymore");
			continue;
		}

		node->SerializeTo(nodesObject.AddGSONObject(""), *func);
	}

	BinaryGSONObject& linksObject = object.AddGSONObject("links");

	for (const ScriptLink& link : func->GetLinks())
	{
		const auto isPinIncluded = [&func, &nodes](const PinId pinId)
			{
				const ScriptPin* pin = func->TryGetPin(pinId);
				return pin != nullptr
					&& std::find(nodes.begin(), nodes.end(), pin->GetNodeId()) != nodes.end();
			};

		if (isPinIncluded(link.GetInputPinId())
			&& isPinIncluded(link.GetOutputPinId()))
		{
			link.SerializeTo(linksObject.AddGSONObject(""));
		}
	}

	std::stringstream str{};
	object.SaveToBinary(str);

	std::string clipBoardText = std::string{ sClipboardScriptIdentifier } + StringFunctions::BinaryToHex(str.str());

	ImGui::SetClipboardText(clipBoardText.c_str());
}

void CE::ScriptEditorSystem::CutSelection()
{
	CopySelection();
	DeleteSelection();
}

void CE::ScriptEditorSystem::DuplicateSelection()
{
	CopySelection();
	Paste(glm::vec2{ 50.0f });
}

void CE::ScriptEditorSystem::Paste(std::optional<glm::vec2> offsetToOldPos)
{
	ScriptFunc* currentFunc = TryGetSelectedFunc();
	const char* clipBoardCStr = ImGui::GetClipboardText();

	if (currentFunc == nullptr
		|| clipBoardCStr == nullptr)
	{
		return;
	}

	std::string_view clipBoardText = clipBoardCStr;

	if (clipBoardText.substr(0, sClipboardScriptIdentifier.size()) != sClipboardScriptIdentifier)
	{
		LOG(LogEditor, Message, "The clipboard does not contain nodes");
		return;
	}

	BinaryGSONObject object{};

	std::string_view hexContent = clipBoardText.substr(sClipboardScriptIdentifier.size());
	std::string binaryContent = StringFunctions::HexToBinary(hexContent);
	view_istream stream{ binaryContent };

	bool success = object.LoadFromBinary(stream);

	const BinaryGSONObject* serializedNodes = object.TryGetGSONObject("nodes");
	const BinaryGSONObject* serializedLinks = object.TryGetGSONObject("links");

	if (!success
		|| serializedNodes == nullptr
		|| serializedLinks == nullptr)
	{
		LOG(LogEditor, Message, "The clipboard contained invalid date");
		return;
	}

	std::vector<std::unique_ptr<ScriptNode>> nodes{};
	std::vector<ScriptLink> links{};
	std::vector<ScriptPin> pins{};

	for (const BinaryGSONObject& serializedLink : serializedLinks->GetChildren())
	{
		std::optional<ScriptLink> optLink = ScriptLink::DeserializeFrom(serializedLink);

		if (!optLink.has_value())
		{
			LOG(LogEditor, Message, "The clipboard contained invalid date");
			return;
		}

		links.emplace_back(std::move(*optLink));
	}

	for (const BinaryGSONObject& serializedNode : serializedNodes->GetChildren())
	{
		nodes.emplace_back(ScriptNode::DeserializeFrom(serializedNode, std::back_inserter(pins), GetClassVersion(MetaManager::Get().GetType<Script>())));

		if (nodes.back() == nullptr)
		{
			LOG(LogEditor, Message, "The clipboard contained invalid date");
			return;
		}
	}

	if (offsetToOldPos.has_value())
	{
		for (std::unique_ptr<ScriptNode>& node : nodes)
		{
			node->SetPosition(node->GetPosition() + *offsetToOldPos);
		}
	}
	else
	{
		glm::vec2 nodesTopLeft{ std::numeric_limits<float>::max() };

		for (const std::unique_ptr<ScriptNode>& node : nodes)
		{
			nodesTopLeft = glm::min(nodesTopLeft, node->GetPosition());
		}

		const glm::vec2 mousePos = ax::NodeEditor::ScreenToCanvas(ImGui::GetMousePos());

		for (std::unique_ptr<ScriptNode>& node : nodes)
		{
			glm::vec2 topLeftToNode = node->GetPosition() - nodesTopLeft;
			node->SetPosition(mousePos + topLeftToNode);
		}
	}

	auto addedNodes = std::get<0>(currentFunc->AddCondensed(std::move(nodes), std::move(links), std::move(pins)));

	ax::NodeEditor::ClearSelection();

	for (const ScriptNode& node : addedNodes)
	{
		// This line is only here to force creation of the node in the
		// backend of ax::NodeEditor
		ax::NodeEditor::SetNodePosition(node.GetId(), node.GetPosition());
		ax::NodeEditor::SelectNode(node.GetId(), true);
	}
}

void CE::ScriptEditorSystem::SaveFunctionState() const
{
	const ScriptFunc* selectedFunc = TryGetSelectedFunc();

	if (selectedFunc == nullptr)
	{
		return;
	}

	ax::NodeEditor::SetCurrentEditor(mContext);
	ImRect canvasRect = ax::NodeEditor::GetSettingsVisibleRect();

	mCanvasRect[selectedFunc->GetName()] = { canvasRect.Min.x, canvasRect.Min.y, canvasRect.Max.x, canvasRect.Max.y };
}

void CE::ScriptEditorSystem::LoadFunctionState()
{
	const ScriptFunc* selectedFunc = TryGetSelectedFunc();

	if (selectedFunc == nullptr)
	{
		return;
	}

	auto it = mCanvasRect.find(selectedFunc->GetName());

	if (it == mCanvasRect.end())
	{
		return;
	}

	ax::NodeEditor::SetCurrentEditor(mContext);

	ax::NodeEditor::SetSettingsVisibleRect(ImRect{ it->second.x, it->second.y, it->second.z, it->second.w });
}

