#include "Precomp.h"
// All the functions and variables are declared in ScriptEditorSystem.h.
// But the functionality has been divided into seperate .cpp files,
// since the amount of code made it hard to find what you needed when
// it was all in one file.
#include "EditorSystems/AssetEditorSystems/ScriptEditorSystem.h"

#include <imgui/imgui_internal.h>
#include <imnodes/examples/blueprints-example/utilities/builders.h>
#include <imnodes/examples/blueprints-example/utilities/widgets.h>

#include "Utilities/Search.h"
#include "Core/Input.h"
#include "Core/VirtualMachine.h"
#include "Scripting/ScriptTools.h"
#include "Scripting/Nodes/CommentScriptNode.h"
#include "Scripting/Nodes/ControlScriptNodes.h"
#include "Scripting/Nodes/EntryAndReturnScriptNode.h"
#include "Scripting/Nodes/MetaFieldScriptNode.h"
#include "Scripting/Nodes/MetaFuncScriptNode.h"
#include "Scripting/Nodes/ReroutScriptNode.h"
#include "Utilities/Imgui/ImguiInspect.h"

static ImRect ImGui_GetItemRect()
{
	return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

static ImRect ImRect_Expanded(const ImRect& rect, float x, float y)
{
	auto result = rect;
	result.Min.x -= x;
	result.Min.y -= y;
	result.Max.x += x;
	result.Max.y += y;
	return result;
}

void CE::ScriptEditorSystem::DisplayCanvas()
{
	if (mContext == nullptr)
	{
		ImGui::TextUnformatted("No function selected");
		return;
	}

	if (!ax::NodeEditor::Begin("Node editor"))
	{
		return;
	}

	// ax::NodeEditor has a bug where sometimes it zooms out to
	// near infinite size. This is a safe-guard against that.
	if (ax::NodeEditor::GetCurrentZoom() > sZoomLevels.back() * 100.0f)
	{
		ax::NodeEditor::NavigateToRect(ImRect{ 0.0f, 0.0f, 100.0f, 100.0f });
	}

	mMousePosInCanvasSpace = ImGui::GetMousePos();

	TryGetSelectedFunc()->PostDeclarationRefresh();

	DrawCanvasObjects();

	if (!mCreateNodePopUpPosition.has_value())
	{
		TryLetUserCreateLink();
	}

	DeleteRequestedItems();

	DisplayCanvasPopUps();

	LinkId doubleClickedLink = ax::NodeEditor::GetDoubleClickedLink();

	if (doubleClickedLink != LinkId::Invalid)
	{
		ScriptFunc& currentFunc = *TryGetSelectedFunc();

		ScriptLink& link = currentFunc.GetLink(doubleClickedLink);

		RerouteScriptNode& node = currentFunc.AddNode<RerouteScriptNode>(currentFunc, currentFunc.GetPin(link.GetOutputPinId()).GetParamInfo());

		ScriptPin& outputPin = currentFunc.GetPin(link.GetOutputPinId());
		ScriptPin& inputPin = currentFunc.GetPin(link.GetInputPinId());

		ax::NodeEditor::SetNodePosition(node.GetId(), ImGui::GetMousePos());

		[[maybe_unused]] ScriptLink* newLink1 = currentFunc.TryAddLink(outputPin, node.GetInputs(currentFunc)[0]);
		[[maybe_unused]] ScriptLink* newLink2 = currentFunc.TryAddLink(inputPin, node.GetOutputs(currentFunc)[0]);
		// Break the original 
		currentFunc.RemoveLink(doubleClickedLink);

		ASSERT(newLink1 != nullptr
			&& newLink2 != nullptr);
	}

	ax::NodeEditor::End();

	for (ScriptNode& node : TryGetSelectedFunc()->GetNodes())
	{
		node.SetPosition(ax::NodeEditor::GetNodePosition(node.GetId()));

		if (node.GetType() == ScriptNodeType::Comment)
		{
			static_cast<CommentScriptNode&>(node).SetSize(ax::NodeEditor::GetGroupSize(node.GetId()));
		}
	}
}

void CE::ScriptEditorSystem::DrawCanvasObjects()
{
	ScriptFunc& currentFunc = *TryGetSelectedFunc();

	ax::NodeEditor::Utilities::BlueprintNodeBuilder builder{};
	std::vector<PinToInspect> pinsToInspect{};

	if (CanAnyPinsBeInspected())
	{
		pinsToInspect.reserve(currentFunc.GetNumOfPinsIncludingRemoved());
	}

	for (ScriptNode& node : currentFunc.GetNodes())
	{
		ax::NodeEditor::SetNodePosition(node.GetId(), node.GetPosition());

		if (node.GetType() == ScriptNodeType::Comment)
		{
			DisplayCommentNode(static_cast<CommentScriptNode&>(node));
		}
		else if (node.GetType() == ScriptNodeType::Rerout)
		{
			DisplayRerouteNode(builder, static_cast<RerouteScriptNode&>(node));
		}
		else
		{
			DisplayFunctionNode(builder, node, pinsToInspect);
		}
	}

	for (const ScriptLink& link : currentFunc.GetLinks())
	{
		const bool hasErrors = !VirtualMachine::Get().GetErrors({ currentFunc, link }).empty();

		const ScriptPin& inputPin = currentFunc.GetPin(link.GetInputPinId());
		const ScriptPin& outputPin = currentFunc.GetPin(link.GetOutputPinId());

		PinId startPin = outputPin.GetId();
		PinId endPin = inputPin.GetId();

		ax::NodeEditor::Link(link.GetId(), startPin, endPin,
			hasErrors ? ImColor{ 1.0f, 0.0f, 0.0f, 1.0f } : GetIconColor(inputPin.GetParamInfo()),
			hasErrors ? 7.0f : 2.0f);
	}

	ax::NodeEditor::Suspend();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });

	const ImRect& canvasRect = ax::NodeEditor::GetCanvasRect();

	const bool bringPinsToFront = Input::Get().HasFocus();

	for (const auto& [pinId, inspectPos] : pinsToInspect)
	{
		ScriptPin& pin = currentFunc.GetPin(pinId);

		if (!canvasRect.Contains({ inspectPos, inspectPos + pin.GetInspectWindowSize() }))
		{
			continue;
		}

		ImGui::SetNextWindowPos(ax::NodeEditor::CanvasToScreen(inspectPos));

		if (ImGui::Begin(Format("PinInspectChild{}", pinId.Get()).c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoBackground
			| ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoFocusOnAppearing))
		{
			if (bringPinsToFront)
			{
				ImGuiWindow* window = ImGui::GetCurrentWindow();
				ImGui::BringWindowToDisplayFront(window);
			}

			InspectPin(currentFunc, pin);
		}

		pin.SetInspectWindowSize(ImGui::GetWindowSize());
		ImGui::End();
	}

	ImGui::PopStyleVar();

	ax::NodeEditor::Resume();
}

void CE::ScriptEditorSystem::TryLetUserCreateLink()
{
	ScriptFunc& currentFunc = *TryGetSelectedFunc();

	if (!ax::NodeEditor::BeginCreate(ImColor(255, 255, 255), 2.0f))
	{
		mPinTheUserIsTryingToLink = ax::NodeEditor::PinId::Invalid;
		ax::NodeEditor::EndCreate();
		return;
	}

	auto showLabel = [](const char* label, ImColor color)
		{
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
			const auto size = ImGui::CalcTextSize(label);

			const auto padding = ImGui::GetStyle().FramePadding;
			const auto spacing = ImGui::GetStyle().ItemSpacing;

			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

			const auto rectMin = ImGui::GetCursorScreenPos() - padding;
			const auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

			const auto drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
			ImGui::TextUnformatted(label);
		};

	ax::NodeEditor::PinId startPinId = 0, endPinId = 0;
	if (ax::NodeEditor::QueryNewLink(&startPinId, &endPinId))
	{
		const ScriptPin* startPin = &currentFunc.GetPin(startPinId);
		const ScriptPin* endPin = &currentFunc.GetPin(endPinId);

		mPinTheUserIsTryingToLink = startPin ? startPin->GetId() : endPin->GetId();

		if (startPin->GetKind() == ScriptPinKind::Input)
		{
			std::swap(startPin, endPin);
			std::swap(startPinId, endPinId);
		}

		if (startPin && endPin)
		{
			if (CanCreateLink(*startPin, *endPin))
			{
				showLabel("+ Create Link", ImColor(32, 45, 32, 180));
				if (ax::NodeEditor::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
				{
					currentFunc.TryAddLink(*startPin, *endPin);
				}
			}
			else
			{
				showLabel("x Incompatible Pins", ImColor(45, 32, 32, 180));
				ax::NodeEditor::RejectNewItem(ImColor(255, 0, 0), 2.0f);
			}
		}
	}

	ax::NodeEditor::PinId pinId = 0;
	if (ax::NodeEditor::QueryNewNode(&pinId))
	{
		mPinTheUserIsTryingToLink = pinId;

		if (currentFunc.TryGetPin(mPinTheUserIsTryingToLink))
		{
			showLabel("+ Create Node", ImColor(32, 45, 32, 180));
		}

		if (ax::NodeEditor::AcceptNewItem())
		{
			ax::NodeEditor::Suspend();
			ImGui::OpenPopup("Create New Node");
			ax::NodeEditor::Resume();
		}
	}

	ax::NodeEditor::EndCreate();
}

void CE::ScriptEditorSystem::AddNewNode(const NodeTheUserCanAdd& nodeToAdd)
{
	ScriptFunc* currentFunc = TryGetSelectedFunc();

	if (currentFunc == nullptr)
	{
		return;
	}

	ScriptNode& node = nodeToAdd.mAddNode(*currentFunc);
	node.SetPosition(mCreateNodePopUpPosition.value_or(mMousePosInCanvasSpace));
	ax::NodeEditor::SetNodePosition(node.GetId(), node.GetPosition());

	if (const ScriptPin* startPin = currentFunc->TryGetPin(mPinTheUserIsTryingToLink);
		startPin != nullptr)
	{
		const Span<const ScriptPin> pins = startPin->IsOutput() ? node.GetInputs(*currentFunc) : node.GetOutputs(*currentFunc);

		for (const ScriptPin& pin : pins)
		{
			if (currentFunc->TryAddLink(*startPin, pin) != nullptr)
			{
				break;
			}
		}
	}

	mCreateNodePopUpPosition.reset();
	mPinTheUserIsTryingToLink = ax::NodeEditor::PinId::Invalid;
}

void CE::ScriptEditorSystem::DeleteRequestedItems()
{
	if (ax::NodeEditor::BeginDelete())
	{
		ax::NodeEditor::NodeId nodeId = 0;
		while (ax::NodeEditor::QueryDeletedNode(&nodeId))
		{
			if (ax::NodeEditor::AcceptDeletedItem())
			{
				TryGetSelectedFunc()->RemoveNode(nodeId);
			}
		}

		ax::NodeEditor::LinkId linkId = 0;
		while (ax::NodeEditor::QueryDeletedLink(&linkId))
		{
			if (ax::NodeEditor::AcceptDeletedItem())
			{
				TryGetSelectedFunc()->RemoveLink(linkId);
			}
		}
	}
	ax::NodeEditor::EndDelete();
}

void CE::ScriptEditorSystem::DisplayCanvasPopUps()
{
	ax::NodeEditor::Suspend();

	if (ax::NodeEditor::ShowPinContextMenu(&mPinTheUserRightClicked))
	{
		ImGui::OpenPopup("Pin Context Menu");
	}
	else if (ax::NodeEditor::ShowBackgroundContextMenu())
	{
		ImGui::OpenPopup("Create New Node");
	}

	DisplayPinContextPopUp();
	DisplayCreateNewNowPopUp(mMousePosInCanvasSpace);

	ax::NodeEditor::Resume();
}

void CE::ScriptEditorSystem::DisplayCreateNewNowPopUp(ImVec2 placeNodeAtPos)
{
	if (!Search::BeginPopup("Create New Node"))
	{
		mCreateNodePopUpPosition.reset();
		return;
	}

	ImGui::SameLine();
	ImGui::Checkbox("Context sensitive", &sContextSensitive);

	if (!mCreateNodePopUpPosition.has_value())
	{
		mCreateNodePopUpPosition = placeNodeAtPos;
	}

	for (const NodeCategory& category : mAllNodesTheUserCanAdd)
	{
		// Only submit the tree node if there are actually items inside of it that we want to show
		if (!std::any_of(category.mNodes.begin(), category.mNodes.end(), [this](const NodeTheUserCanAdd& node) { return ShouldShowUserNode(node); }))
		{
			continue;
		}

		Search::TreeNode(category.mName);
		Search::SetBonus(category.mSearchBonus);

		for (const NodeTheUserCanAdd& node : category.mNodes)
		{
			if (!ShouldShowUserNode(node))
			{
				continue;
			}

			if (Search::Button(node.mName))
			{
				ImGui::CloseCurrentPopup();
				AddNewNode(node);
			}

			Search::SetBonus(category.mSearchBonus);
		}

		Search::TreePop();
	}

	Search::EndPopup();
}

void CE::ScriptEditorSystem::DisplayPinContextPopUp()
{
	if (!ImGui::BeginPopup("Pin Context Menu"))
	{
		return;
	}

	ScriptFunc& currentFunc = *TryGetSelectedFunc();
	const ScriptPin* pin = currentFunc.TryGetPin(mPinTheUserRightClicked);

	if (pin == nullptr)
	{
		mPinTheUserRightClicked = ax::NodeEditor::PinId::Invalid;
		ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
		return;
	}

	ImGui::Text("Type: %s", pin->GetTypeName().c_str());
	ImGui::Text("Form: %s", EnumToString(pin->GetTypeForm()).data());

	const std::vector<std::reference_wrapper<ScriptLink>> linksConnectedToPin = currentFunc.GetAllLinksConnectedToPin(mPinTheUserRightClicked);

	if (!linksConnectedToPin.empty())
	{
		auto getNodeOfOtherPin = [&](const ScriptLink& link) -> decltype(auto)
			{
				const ScriptPin& otherPin = currentFunc.GetPin(pin->IsInput() ? link.GetOutputPinId() : link.GetInputPinId());
				return currentFunc.GetNode(otherPin.GetNodeId());
			};

		bool shouldDisplay = true;

		if (linksConnectedToPin.size() > 1)
		{
			shouldDisplay = ImGui::BeginMenu("Break link");
		}

		if (shouldDisplay)
		{
			for (const ScriptLink& link : linksConnectedToPin)
			{
				if (ImGui::Button(Format("Break link to {}", getNodeOfOtherPin(link).GetTitle()).c_str()))
				{
					currentFunc.RemoveLink(link.GetId());
				}
			}

			if (linksConnectedToPin.size() > 1)
			{
				if (ImGui::Button("Break all links"))
				{
					for (const ScriptLink& link : linksConnectedToPin)
					{
						currentFunc.RemoveLink(link.GetId());
					}
				}
				ImGui::EndMenu();
			}
		}
	}

	ImGui::EndPopup();
}

void CE::ScriptEditorSystem::DisplayFunctionNode(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder,
	ScriptNode& node, std::vector<PinToInspect>& pinsToInspect)
{
	ScriptFunc& currentFunc = *TryGetSelectedFunc();
	const bool hasErrors = !VirtualMachine::Get().GetErrors({ currentFunc, node }).empty();

	ImGui::PushStyleColor(ax::NodeEditor::StyleColor_Bg,
		hasErrors ? ImVec4{ 1.0f, 0.0f, 0.0f, 1.0f } : ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });

	builder.Begin(node.GetId());

	builder.Header(node.GetHeaderColor(currentFunc));

	ImGui::SetWindowFontScale(1.5f);
	ImGui::Spring(0.0f);
	ImGui::TextUnformatted(node.GetTitle().c_str());

	ImGui::SetWindowFontScale(1.0f);

	builder.EndHeader();

	ImGui::PopStyleColor();

	for (ScriptPin& pin : node.GetPins(currentFunc))
	{
		DisplayPin(builder, pin, pinsToInspect);
	}

	builder.End();
}

bool CE::ScriptEditorSystem::IsRerouteNodeFlipped(const RerouteScriptNode& node) const
{
	const ScriptFunc& currentFunc = *TryGetSelectedFunc();

	const std::vector<std::reference_wrapper<const ScriptLink>> linksConnectedToInput = currentFunc.GetAllLinksConnectedToPin(node.GetInputs(currentFunc)[0].GetId());
	const std::vector<std::reference_wrapper<const ScriptLink>> linksConnectedToOutput = currentFunc.GetAllLinksConnectedToPin(node.GetOutputs(currentFunc)[0].GetId());

	for (const ScriptLink& linkConnectedToInput : linksConnectedToInput)
	{
		const ScriptNode& nodeConnectedToInput = currentFunc.GetNode(currentFunc.GetPin(linkConnectedToInput.GetOutputPinId()).GetNodeId());

		const float connectedToInputX = ax::NodeEditor::GetNodePosition(nodeConnectedToInput.GetId()).x + ax::NodeEditor::GetNodeSize(nodeConnectedToInput.GetId()).x;

		for (const ScriptLink& linkConnectedToOutput : linksConnectedToOutput)
		{
			const ScriptNode& nodeConnectedToOutput = currentFunc.GetNode(currentFunc.GetPin(linkConnectedToOutput.GetInputPinId()).GetNodeId());
			const float connectedToOutputX = ax::NodeEditor::GetNodePosition(nodeConnectedToOutput.GetId()).x;

			if (connectedToInputX > connectedToOutputX)
			{
				return true;
			}
		}
	}

	return false;
}

void CE::ScriptEditorSystem::DisplayRerouteNode(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder,
	RerouteScriptNode& node)
{
	const ScriptFunc& currentFunc = *TryGetSelectedFunc();
	ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBg, glm::vec4{ GetIconColor(node.GetInputs(currentFunc)[0].GetParamInfo()).Value }*.6f);

	builder.Begin(node.GetId(), ImVec4(0, 0, 0, 0));

	const bool flipped = IsRerouteNodeFlipped(node);

	ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_SourceDirection, flipped ? ImVec2{ -1.0f, 0.0f } : ImVec2{ 1.0f, 0.0f });
	ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_TargetDirection, flipped ? ImVec2{ 1.0f, 0.0f } : ImVec2{ -1.0f, 0.0f });

	auto drawPin = [&](const ScriptPin& pin)
		{
			float alpha = ImGui::GetStyle().Alpha;

			if (const ScriptPin* pinTheUserIsTringToLink = currentFunc.TryGetPin(mPinTheUserIsTryingToLink);
				pinTheUserIsTringToLink != nullptr && CanCreateLink(*pinTheUserIsTringToLink, pin))
			{
				alpha = alpha * (48.0f / 255.0f);
			}

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

			ax::NodeEditor::BeginPin(pin.GetId(), pin.GetKind() == ScriptPinKind::Input ? ax::NodeEditor::PinKind::Input : ax::NodeEditor::PinKind::Output);

			DrawPinIcon(pin, pin.IsLinked(), static_cast<int>(alpha * 255.0f), flipped);

			ax::NodeEditor::EndPin();

			ImGui::PopStyleVar();
		};


	if (flipped)
	{
		drawPin(node.GetOutputs(currentFunc)[0]);
		ImGui::SameLine();
		drawPin(node.GetInputs(currentFunc)[0]);
	}
	else
	{
		drawPin(node.GetInputs(currentFunc)[0]);
		ImGui::SameLine();
		drawPin(node.GetOutputs(currentFunc)[0]);
	}


	ax::NodeEditor::PopStyleVar(2);
	builder.End();

	ax::NodeEditor::PopStyleColor();
}

bool CE::ScriptEditorSystem::CanAnyPinsBeInspected() const
{
	return std::abs(ax::NodeEditor::GetCurrentZoom() - 1.0f) < .2f;
}

void CE::ScriptEditorSystem::DisplayCommentNode(CommentScriptNode& node)
{
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.75f);
	PushStyleColor(ax::NodeEditor::StyleColor_NodeBg, node.GetColour());
	PushStyleColor(ax::NodeEditor::StyleColor_NodeBorder, node.GetColour() * .7f);

	ax::NodeEditor::BeginNode(node.GetId());
	ImGui::PushID(static_cast<int32>(node.GetId().Get()));

	ImGui::TextUnformatted(node.GetComment().c_str());

	const ImVec2 nodeDesiredSize = ImMax({ 50.0f, 50.0f }, ImVec2{ node.GetSize() });
	ax::NodeEditor::Group(nodeDesiredSize);

	ImGui::PopID();
	ax::NodeEditor::EndNode();
	ax::NodeEditor::PopStyleColor(2);
	ImGui::PopStyleVar();

	if (ax::NodeEditor::BeginGroupHint(node.GetId()))
	{
		const auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);

		const auto min = ax::NodeEditor::GetGroupMin();

		ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));

		ImGui::BeginGroup();
		ImGui::TextUnformatted(node.GetDisplayName().c_str());
		ImGui::EndGroup();

		const auto drawList = ax::NodeEditor::GetHintBackgroundDrawList();

		const auto hintBounds = ImGui_GetItemRect();
		const auto hintFrameBounds = ImRect_Expanded(hintBounds, 8, 4);

		drawList->AddRectFilled(
			hintFrameBounds.GetTL(),
			hintFrameBounds.GetBR(),
			IM_COL32(255, 255, 255, 32 * bgAlpha / 255), 4.0f);

		drawList->AddRect(
			hintFrameBounds.GetTL(),
			hintFrameBounds.GetBR(),
			IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f);
	}
	ax::NodeEditor::EndGroupHint();
}


void CE::ScriptEditorSystem::DisplayPin(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder,
	ScriptPin& pin, std::vector<PinToInspect>& pinsToInspect)
{
	const ScriptFunc& currentFunc = *TryGetSelectedFunc();

	float alpha = ImGui::GetStyle().Alpha;

	if (const ScriptPin* pinTheUserIsTringToLink = currentFunc.TryGetPin(mPinTheUserIsTryingToLink);
		pinTheUserIsTringToLink != nullptr && CanCreateLink(*pinTheUserIsTringToLink, pin))
	{
		alpha = alpha * (48.0f / 255.0f);
	}

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

	if (pin.IsInput())
	{
		builder.Input(pin.GetId());
	}
	else
	{
		builder.Output(pin.GetId());
	}

	const glm::vec2 topLeft = ImGui::GetCursorPos();
	const std::string& pinName = pin.GetName();

	if (pin.IsInput())
	{
		//const glm::vec2 textStart = topLeft + glm::vec2{ 2.0f + static_cast<float>(sPinIconSize), 5.0f };

		ImGui::SetCursorPos(topLeft + glm::vec2{ ImGui::CalcTextSize(pinName.c_str()).x + static_cast<float>(sPinIconSize), 0.0f });

		const glm::vec2 inspectWindowSize = glm::max(sMinInspectPinWindowSize, pin.GetInspectWindowSize());

		if (CanInspectPin(currentFunc, pin))
		{
			if (CanAnyPinsBeInspected())
			{
				pinsToInspect.emplace_back(PinToInspect{ pin.GetId(), ImGui::GetCursorPos() + glm::vec2{ 2.0f } });
			}

			ImGui::Dummy(inspectWindowSize);
		}

		const glm::vec2 tempCursorPos = ImGui::GetCursorPos();

		const glm::vec2 iconPosition = { topLeft.x, topLeft.y + (inspectWindowSize.y * .5f) - (static_cast<float>(sPinIconSize) * .5f) };

		ImGui::SetCursorPos(iconPosition);
		DrawPinIcon(pin, pin.IsLinked(), (int)(alpha * 255));

		ImGui::SetCursorPos(iconPosition + glm::vec2{ static_cast<float>(sPinIconSize) + 2.0f, 5.0f });

		ImGui::TextUnformatted(pinName.c_str());

		ImGui::SetCursorPos(tempCursorPos);

		builder.EndInput();
	}
	else
	{
		const glm::vec2 textStart = topLeft + glm::vec2{ 2.0f, 0.0f };

		ImGui::SetCursorPos(textStart);
		ImGui::TextUnformatted(pinName.c_str());
		ImGui::SetCursorPos(textStart + ImVec2{ ImGui::CalcTextSize(pinName.c_str()).x, -2.0f });

		DrawPinIcon(pin, pin.IsLinked(), (int)(alpha * 255));
		builder.EndOutput();
	}

	ImGui::PopStyleVar();
}

bool CE::ScriptEditorSystem::ShouldWeOnlyShowContextMatchingNodes() const
{
	return sContextSensitive && TryGetSelectedFunc()->TryGetPin(mPinTheUserIsTryingToLink) != nullptr;
}

bool CE::ScriptEditorSystem::DoesNodeMatchContext(const ScriptPin& contextPin,
	TypeTraits returnTypeTraits,
	const std::vector<TypeTraits>& parameters,
	bool isPure)
{
	if (contextPin.IsFlow())
	{
		return !isPure;
	}

	const MetaType* const contextType = contextPin.TryGetType();

	if (contextType == nullptr)
	{
		return false;
	}

	const TypeTraits contextTraits{ contextType->GetTypeId(), contextPin.GetTypeForm() };

	if (contextPin.IsOutput())
	{
		return std::any_of(parameters.begin(), parameters.end(),
			[contextTraits](TypeTraits param)
			{
				return !MetaFunc::CanArgBePassedIntoParam(contextTraits, param).has_value();
			});
	}
	return !MetaFunc::CanArgBePassedIntoParam({ returnTypeTraits.mStrippedTypeId, returnTypeTraits.mForm }, contextTraits).has_value();
}

bool CE::ScriptEditorSystem::DoesNodeMatchContext(const NodeTheUserCanAdd& node) const
{
	return !ShouldWeOnlyShowContextMatchingNodes() || node.mMatchesContext(TryGetSelectedFunc()->GetPin(mPinTheUserIsTryingToLink));
}

bool CE::ScriptEditorSystem::ShouldShowUserNode(const NodeTheUserCanAdd& node) const
{
	return DoesNodeMatchContext(node);
}

void CE::ScriptEditorSystem::InitialiseAllNodesTheUserCanAdd()
{
	NodeCategory& common = mAllNodesTheUserCanAdd.emplace_back(NodeCategory{ "Common" });

	common.mNodes.emplace_back(std::string{ BranchScriptNode::sBranchNodeName },
		[](ScriptFunc& func) -> decltype(auto)
		{
			return func.AddNode<BranchScriptNode>(func);
		},
		[](const ScriptPin& contextPin) -> bool
		{
			return contextPin.IsFlow()
				|| (contextPin.TryGetType() != nullptr && contextPin.TryGetType()->GetTypeId() == MakeTypeId<bool>() && contextPin.IsOutput());
		},
		[](const ScriptNode& node) -> bool
		{
			return node.GetType() == ScriptNodeType::Branch;
		},
		Input::KeyboardKey::B);


	common.mNodes.emplace_back(std::string{ ForLoopScriptNode::sForLoopNodeName },
		[](ScriptFunc& func) -> decltype(auto)
		{
			return func.AddNode<ForLoopScriptNode>(func);
		},
		[](const ScriptPin& contextPin) -> bool
		{
			return contextPin.IsFlow()
				|| (contextPin.TryGetType() != nullptr && contextPin.TryGetType()->GetTypeId() == MakeTypeId<int32>());
		},
		[](const ScriptNode& node) -> bool
		{
			return node.GetType() == ScriptNodeType::ForLoop;
		}
		);

	common.mNodes.emplace_back(std::string{ WhileLoopScriptNode::sWhileLoopNodeName },
		[](ScriptFunc& func) -> decltype(auto)
		{
			return func.AddNode<WhileLoopScriptNode>(func);
		},
		[](const ScriptPin& contextPin) -> bool
		{
			return contextPin.IsFlow()
				|| (contextPin.TryGetType() != nullptr && contextPin.TryGetType()->GetTypeId() == MakeTypeId<bool>() && contextPin.IsOutput());
		},
		[](const ScriptNode& node) -> bool
		{
			return node.GetType() == ScriptNodeType::WhileLoop;
		});

	common.mNodes.emplace_back(std::string{ FunctionEntryScriptNode::sEntryNodeName },
		[this](ScriptFunc& func) -> decltype(auto)
		{
			return func.AddNode<FunctionEntryScriptNode>(func, mAsset);
		},
		[this](const ScriptPin& contextPin) -> bool
		{
			const ScriptFunc* const currentFunc = TryGetSelectedFunc();

			if (currentFunc == nullptr)
			{
				return false;
			}

			// The entry node is the only node that has multiple output pins,
			// so we iterate over all of them
			for (const ScriptVariableTypeData& param : currentFunc->GetParameters(true))
			{
				if (param.TryGetType() == nullptr)
				{
					continue;
				}

				if (DoesNodeMatchContext(contextPin, { param.TryGetType()->GetTypeId(), param.GetTypeForm() }, {}, currentFunc->IsPure()))
				{
					return true;
				}
			}
			return false;
		},
		[](const ScriptNode& node) -> bool
		{
			return node.GetType() == ScriptNodeType::FunctionEntry;
		});


	common.mNodes.emplace_back(std::string{ FunctionReturnScriptNode::sReturnNodeName },
		[this](ScriptFunc& func) -> decltype(auto)
		{
			return func.AddNode<FunctionReturnScriptNode>(func, mAsset);
		},
		[this](const ScriptPin& contextPin) -> bool
		{
			const ScriptFunc* const currentFunc = TryGetSelectedFunc();

			if (currentFunc == nullptr)
			{
				return false;
			}

			const std::optional<ScriptVariableTypeData>& returnType = currentFunc->GetReturnType();

			if (!returnType.has_value()
				|| returnType->TryGetType() == nullptr)
			{
				return false;
			}

			return DoesNodeMatchContext(contextPin, {}, { { returnType->TryGetType()->GetTypeId(), returnType->GetTypeForm() } }, currentFunc->IsPure());
		},
		[](const ScriptNode& node) -> bool
		{
			return node.GetType() == ScriptNodeType::FunctionReturn;
		});

	common.mNodes.emplace_back("Comment",
		[](ScriptFunc& func) -> decltype(auto)
		{
			return func.AddNode<CommentScriptNode>(func, "New comment");
		},
		[](const ScriptPin&) -> bool
		{
			return true;
		},
		[](const ScriptNode& node) -> bool
		{
			return node.GetType() == ScriptNodeType::Comment;
		},
		Input::KeyboardKey::K);

	auto addType = [&](const MetaType& type, float categoryBonus)
		{
			NodeCategory typeCategory{ type.GetName(), categoryBonus };

			for (const MetaFunc& metaFunc : type.GetDirectFuncs())
			{
				if (!CanFunctionBeTurnedIntoNode(metaFunc))
				{
					continue;
				}

				typeCategory.mNodes.emplace_back(std::string{ metaFunc.GetDesignerFriendlyName() },
					[&type, &metaFunc](ScriptFunc& func) -> decltype(auto)
					{
						return func.AddNode<MetaFuncScriptNode>(func, type, metaFunc);
					},
					[&metaFunc](const ScriptPin& contextPin) -> bool
					{
						return DoesNodeMatchContext(contextPin,
						metaFunc.GetReturnType().mTypeTraits,
						[&]
							{
								std::vector<TypeTraits> params{};
								params.reserve(metaFunc.GetParameters().size());

								for (const MetaFuncNamedParam& namedParam : metaFunc.GetParameters())
								{
									params.emplace_back(namedParam.mTypeTraits);
								}

								return params;
							}(),
								IsFunctionPure(metaFunc));
					},
					[&metaFunc](const ScriptNode& node) -> bool
					{
						return node.GetType() == ScriptNodeType::FunctionCall
							&& static_cast<const MetaFuncScriptNode&>(node).TryGetOriginalFunc() == &metaFunc;
					});
			}

			for (const MetaField& field : type.GetDirectFields())
			{
				if (CanBeGetThroughScripts(field, false))
				{
					typeCategory.mNodes.emplace_back(GetterScriptNode::GetTitle(field.GetName(), true),
						[&field](ScriptFunc& func) -> decltype(auto)
						{
							return func.AddNode<GetterScriptNode>(func, field, true);
						},
						[&field](const ScriptPin& contextPin) -> bool
						{
							return DoesNodeMatchContext(contextPin, { field.GetType().GetTypeId(), TypeForm::Value }, { { field.GetOuterType().GetTypeId(), TypeForm::ConstRef } }, true);
						},
						[&field](const ScriptNode& node) -> bool
						{
							return node.GetType() == ScriptNodeType::Getter
								&& static_cast<const GetterScriptNode&>(node).TryGetOriginalField() == &field
								&& static_cast<const GetterScriptNode&>(node).DoesNodeReturnCopy();
						});
				}

				if (CanBeGetThroughScripts(field, true))
				{
					typeCategory.mNodes.emplace_back(GetterScriptNode::GetTitle(field.GetName(), false),
						[&field](ScriptFunc& func) -> decltype(auto)
						{
							return func.AddNode<GetterScriptNode>(func, field, false);
						},
						[&field](const ScriptPin& contextPin) -> bool
						{
							return DoesNodeMatchContext(contextPin, { field.GetType().GetTypeId(), TypeForm::Ref }, { { field.GetOuterType().GetTypeId(), TypeForm::Ref } }, true);
						},
						[&field](const ScriptNode& node) -> bool
						{
							return node.GetType() == ScriptNodeType::Getter
								&& static_cast<const GetterScriptNode&>(node).TryGetOriginalField() == &field
								&& !static_cast<const GetterScriptNode&>(node).DoesNodeReturnCopy();
						});
				}

				if (CanBeSetThroughScripts(field))
				{
					typeCategory.mNodes.emplace_back(Format("Set {}", field.GetName()),
						[&field](ScriptFunc& func) -> decltype(auto)
						{
							return func.AddNode<SetterScriptNode>(func, field);
						},
						[&field](const ScriptPin& contextPin) -> bool
						{
							return DoesNodeMatchContext(contextPin,
								{ field.GetType().GetTypeId() },
								{ { field.GetOuterType().GetTypeId()},{ field.GetType().GetTypeId() } }, false);
						},
						[&field](const ScriptNode& node) -> bool
						{
							return node.GetType() == ScriptNodeType::Setter
								&& static_cast<const SetterScriptNode&>(node).TryGetOriginalField() == &field;
						});
				}
			}

			if (!typeCategory.mNodes.empty())
			{
				std::sort(typeCategory.mNodes.begin(), typeCategory.mNodes.end(),
					[](const NodeTheUserCanAdd& lhs, const NodeTheUserCanAdd& rhs)
					{
						return lhs.mName < rhs.mName;
					});

				mAllNodesTheUserCanAdd.emplace_back(std::move(typeCategory));
			}
		};

	const MetaType* myType = MetaManager::Get().TryGetType(mAsset.GetName());

	if (myType != nullptr)
	{
		addType(*myType, sSearchBonusIncreaseForThisScript);
	}

	for (const MetaType& type : MetaManager::Get().EachType())
	{
		if (&type != myType)
		{
			addType(type, 0.0f);
		}
	}

	// In the VERY rare case in which there were no types reflected
	if (mAllNodesTheUserCanAdd.size() == 1)
	{
		return;
	}

	// + 1, Because we want to leave the Common category at the top, always
	// And if our type was not nullptr, we want to leave that in place as well
	std::sort(mAllNodesTheUserCanAdd.begin() + 1 + (myType != nullptr), mAllNodesTheUserCanAdd.end(),
		[](const NodeCategory& lhs, const NodeCategory& rhs)
		{
			return lhs.mName < rhs.mName;
		});

	mNodePopularityCalculateThread =
	{
		[this]()
		{
			uint32 totalNumOfNodesUsed{};

			for (const WeakAssetHandle<Script> asset : AssetManager::Get().GetAllAssets<Script>())
			{
				if (mShouldWeStopCountingNodePopularity)
				{
					break;
				}

				AssetHandle<Script> script{ asset };

				for (const ScriptFunc& func : script->GetFunctions())
				{
					if (mShouldWeStopCountingNodePopularity)
					{
						break;
					}

					for (const ScriptNode& node : func.GetNodes())
					{
						if (mShouldWeStopCountingNodePopularity)
						{
							break;
						}

						for (NodeCategory& nodeCategory : mAllNodesTheUserCanAdd)
						{
							if (mShouldWeStopCountingNodePopularity)
							{
								break;
							}

							for (NodeTheUserCanAdd& nodeThatCanBeAdded : nodeCategory.mNodes)
							{
								if (mShouldWeStopCountingNodePopularity)
								{
									break;
								}

								if (nodeThatCanBeAdded.mWasCreatedFromThis(node))
								{
									nodeThatCanBeAdded.mNumOfTimesUsed++;
									totalNumOfNodesUsed++;
								}
							}
						}
					}
				}
			}

			for (NodeCategory& nodeCategory : mAllNodesTheUserCanAdd)
			{
				if (mShouldWeStopCountingNodePopularity)
				{
					break;
				}

				for (NodeTheUserCanAdd& nodeThatCanBeAdded : nodeCategory.mNodes)
				{
					if (mShouldWeStopCountingNodePopularity)
					{
						break;
					}

					nodeThatCanBeAdded.mSearchBonus += sPopularityInfluenceOnSearchBonus * static_cast<float>(nodeThatCanBeAdded.mNumOfTimesUsed) / static_cast<float>(totalNumOfNodesUsed);
				}
			}
		}
	};
}

ImColor CE::ScriptEditorSystem::GetIconColor(const ScriptVariableTypeData& typeData) const
{
	if (typeData.IsFlow())
	{
		return ImColor(255, 255, 255);
	}

	if (typeData.TryGetType() == nullptr)
	{
		return ImColor(255, 0, 0);
	}

	switch (typeData.TryGetType()->GetTypeId())
	{
	case MakeTypeId<bool>():
		return ImColor(220, 48, 48);
	case MakeTypeId<int8>():
	case MakeTypeId<int16>():
	case MakeTypeId<int32>():
	case MakeTypeId<int64>():
	case MakeTypeId<uint8>():
	case MakeTypeId<uint16>():
	case MakeTypeId<uint32>():
	case MakeTypeId<uint64>():
		return ImColor(68, 201, 156);
	case MakeTypeId<glm::vec2>():
	case MakeTypeId<glm::vec3>():
	case MakeTypeId<glm::vec4>():
		return ImColor(253, 200, 34);
	case MakeTypeId<float32>():
	case MakeTypeId<float64>():
		return ImColor(147, 226, 74);
	case MakeTypeId<std::string>():
		return ImColor(124, 21, 153);
	default:
		return ImColor(51, 150, 215);
	}
}

void CE::ScriptEditorSystem::DrawPinIcon(const ScriptPin& pin, bool connected, int alpha, bool mirrored) const
{
	const ax::Widgets::IconType iconType = pin.IsFlow() ? ax::Widgets::IconType::Flow : ax::Widgets::IconType::Circle;

	const bool hasErrors = !VirtualMachine::Get().GetErrors({ *TryGetSelectedFunc(), pin }).empty();
	ImColor color = hasErrors ? ImColor(1.0f, 0.0f, 0.0f, 1.0f) : GetIconColor(pin.GetParamInfo());
	color.Value.w = alpha / 255.0f;

	ax::Widgets::Icon(ImVec2(static_cast<float>(sPinIconSize), static_cast<float>(sPinIconSize)), iconType, connected, mirrored, color, ImColor(32, 32, 32, alpha));
}

