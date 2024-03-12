#include "Precomp.h"
// All the functions and variables are declared in ScriptEditorSystem.h.
// But the functionality has been divided into seperate .cpp files,
// since the amount of code made it hard to find what you needed when
// it was all in one file.
#include "EditorSystems/AssetEditorSystems/ScriptEditorSystem.h"
#include "Scripting/ScriptEvents.h"

#include "Utilities/Imgui/ImguiHelpers.h"

void Engine::ScriptEditorSystem::DisplayClassPanel()
{
	ImGui::BeginDisabled(mContext == nullptr);

	if (ImGui::Button("Zoom to Content"))
		ax::NodeEditor::NavigateToContent();

	ImGui::SameLine();

	if (ImGui::Button("Show Flow"))
	{
		const ScriptFunc& currentFunc = *TryGetSelectedFunc();

		for (const ScriptLink& link : currentFunc.GetLinks())
		{
			const ScriptPin* const inputPin = currentFunc.TryGetPin(link.GetInputPinId());
			const ScriptPin* const outputPin = currentFunc.TryGetPin(link.GetOutputPinId());

			if (inputPin == nullptr
				|| outputPin == nullptr
				|| !inputPin->IsFlow()
				|| !outputPin->IsFlow())
			{
				continue;
			}

			ax::NodeEditor::Flow(link.GetId());
		}
	}

	ImGui::EndDisabled();

	DisplayEventsOverview();
	DisplayFunctionsOverview();
	DisplayMembersOverview();
}

void Engine::ScriptEditorSystem::DisplayEventsOverview()
{
	std::vector<Name> functionNames{};

	for (const ScriptFunc& func : mAsset.GetFunctions())
	{
		if (func.IsEvent())
		{
			functionNames.emplace_back(func.GetName());
		}
	}

	const OverviewResult result = DisplayOverview("Events", std::move(functionNames),
		TryGetSelectedFunc() == nullptr ? std::optional<Name>{} : TryGetSelectedFunc()->GetName());

	if (result.mAddButtonPressed)
	{
		ImGui::OpenPopup("AddEventPopUp");
	}

	if (ImGui::BeginPopup("AddEventPopUp"))
	{
		for (const ScriptEvent& event : sAllScriptableEvents)
		{
			if (mAsset.TryGetFunc(event.mBasedOnEvent.get().mName) != nullptr)
			{
				continue;
			}

			if (ImGui::Button(event.mBasedOnEvent.get().mName.data()))
			{
				SelectFunction(&mAsset.AddEvent(event));
			}
		}

		ImGui::EndPopup();
	}

	if (result.mDeleteItemWithThisName.has_value())
	{
		const Name nameOfCurrentFunc = TryGetSelectedFunc() != nullptr ? TryGetSelectedFunc()->GetName() : Name{ 0u };

		SelectFunction(nullptr);
		mAsset.RemoveFunc(*result.mDeleteItemWithThisName);
		SelectFunction(mAsset.TryGetFunc(nameOfCurrentFunc));
	}

	if (result.mSwitchedToName.has_value())
	{
		SelectFunction(mAsset.TryGetFunc(*result.mSwitchedToName));
	}
}

void Engine::ScriptEditorSystem::DisplayFunctionsOverview()
{
	std::vector<Name> functionNames{};

	for (const ScriptFunc& func : mAsset.GetFunctions())
	{
		if (!func.IsEvent())
		{
			functionNames.emplace_back(func.GetName());
		}
	}

	const OverviewResult result = DisplayOverview("Functions", std::move(functionNames),
		TryGetSelectedFunc() == nullptr ? std::optional<Name>{} : TryGetSelectedFunc()->GetName());

	if (result.mAddButtonPressed)
	{
		std::string name = "New function";

		while (mAsset.TryGetFunc(name) != nullptr)
		{
			name.append(" (1)");
		}

		SelectFunction(&mAsset.AddFunc(name));
	}

	if (result.mDeleteItemWithThisName.has_value())
	{
		const Name nameOfCurrentFunc = TryGetSelectedFunc() != nullptr ? TryGetSelectedFunc()->GetName() : Name{ 0u };

		SelectFunction(nullptr);
		mAsset.RemoveFunc(*result.mDeleteItemWithThisName);
		SelectFunction(mAsset.TryGetFunc(nameOfCurrentFunc));
	}

	if (result.mSwitchedToName.has_value())
	{
		SelectFunction(mAsset.TryGetFunc(*result.mSwitchedToName));
	}
}

void Engine::ScriptEditorSystem::DisplayMembersOverview()
{
	std::vector<Name> memberNames{};

	for (const ScriptField& field : mAsset.GetFields())
	{
		memberNames.emplace_back(field.GetName());
	}

	const OverviewResult result = DisplayOverview("Members", std::move(memberNames),
		TryGetSelectedField() == nullptr ? std::optional<Name>{} : TryGetSelectedField()->GetName());

	if (result.mAddButtonPressed)
	{
		std::string name = "New field";

		while (mAsset.TryGetField(name) != nullptr)
		{
			name.append(" (1)");
		}

		SelectField(&mAsset.AddField(name));
	}

	if (result.mDeleteItemWithThisName.has_value())
	{
		const Name nameOfCurrentMember = TryGetSelectedField() != nullptr ? TryGetSelectedField()->GetName() : Name{ 0u };

		SelectField(nullptr);
		mAsset.RemoveField(*result.mDeleteItemWithThisName);
		SelectField(mAsset.TryGetField(nameOfCurrentMember));
	}

	if (result.mSwitchedToName.has_value())
	{
		SelectField(mAsset.TryGetField(*result.mSwitchedToName));
	}
}

Engine::ScriptEditorSystem::OverviewResult Engine::ScriptEditorSystem::DisplayOverview(const char* label,
	std::vector<Name> names,
	const std::optional<Name> nameOfCurrentlySelected)
{
	OverviewResult result{};

	if (ImGui::CollapsingHeaderWithButton(label, "+", &result.mAddButtonPressed, ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (const Name& name : names)
		{
			// Cannot call with an empty name, this prevents that
			if (ImGui::Selectable(Format("{}##NonEmpty", name.StringView()).c_str(), name == nameOfCurrentlySelected.value_or(Name{ 0u })))
			{
				result.mSwitchedToName = name;
			}

			if (ImGui::BeginPopupContextItem(name.CString()))
			{
				if (ImGui::Button("Delete"))
				{
					result.mDeleteItemWithThisName = name;
				}

				ImGui::EndPopup();
			}
		}
	}

	return result;
}
