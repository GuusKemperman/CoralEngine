#include "Precomp.h"
// All the functions and variables are declared in ScriptEditorSystem.h.
// But the functionality has been divided into seperate .cpp files,
// since the amount of code made it hard to find what you needed when
// it was all in one file.
#include "BasicDataTypes/Colors/LinearColor.h"
#include "EditorSystems/AssetEditorSystems/ScriptEditorSystem.h"

#include "Core/VirtualMachine.h"
#include "Meta/MetaTypeFilter.h"
#include "Scripting/ScriptTools.h"
#include "Utilities/Imgui/ImguiInspect.h"
#include "Scripting/Nodes/CommentScriptNode.h"
#include "Utilities/Events.h"

namespace CE
{
	struct ParamWrapper
	{
		ParamWrapper() = default;
		ParamWrapper(const ScriptVariableTypeData& param) : mParam(param) {}
		ParamWrapper(ScriptVariableTypeData&& param) : mParam(std::move(param)) {}
		ScriptVariableTypeData mParam;

		bool operator==(const ParamWrapper& other) const { return mParam == other.mParam; }
		bool operator!=(const ParamWrapper& other) const { return mParam != other.mParam; }

		void DisplayInspectUI(const std::string& widgetName);
	};
}

IMGUI_AUTO_DEFINE_INLINE(template<>, CE::ParamWrapper, var.DisplayInspectUI(name);)

void CE::ScriptEditorSystem::DisplayDetailsPanel()
{
	ScriptFunc* func = TryGetSelectedFunc();
	ScriptField* field = TryGetSelectedField();

	if (field != nullptr)
	{
		DisplayMemberDetails(*field);
	}
	else if (func != nullptr)
	{
		std::vector<NodeId> selectedNodes = GetSelectedNodes();

		if (selectedNodes.size() == 1)
		{
			ScriptNode* const node = func->TryGetNode(selectedNodes[0]);

			if (node != nullptr)
			{
				LIKELY;
				DisplayNodeDetails(*node);
			}
			else
			{
				LOG(LogEditor, Warning, "Selecting a node that no longer exists, should not be possible");
			}
		}
		else if (func->IsEvent())
		{
			DisplayEventDetails(*func);
		}
		else
		{
			DisplayFunctionDetails(*func);
		}
	}
}


void CE::ScriptEditorSystem::DisplayFunctionDetails(ScriptFunc& func)
{
	std::string funcName = func.GetName();

	if (ShowInspectUI("##FuncName", funcName))
	{
		func.SetName(funcName);
	}

	bool isPure = func.IsPure();
	if (ShowInspectUI("IsPure", isPure))
	{
		func.SetIsPure(isPure);
	}

	bool isStatic = func.IsStatic();
	if (ShowInspectUI("IsStatic", isStatic))
	{
		func.SetIsStatic(isStatic);
	}

	std::vector<ParamWrapper> paramWrappers{};
	std::vector<ScriptVariableTypeData> params = func.GetParameters(true);
	for (ScriptVariableTypeData& param : params)
	{
		paramWrappers.emplace_back(std::move(param));
	}
	params.clear();

	if (ShowInspectUI("Inputs", paramWrappers))
	{
		for (ParamWrapper& paramWrapper : paramWrappers)
		{
			// The parameter was just added
			if (paramWrapper.mParam.GetTypeName().empty())
			{
				paramWrapper.mParam = { MakeTypeTraits<int>(), "New input" };
			}

			params.emplace_back(paramWrapper.mParam);
		}

		func.SetParameters(std::move(params));
	}

	const std::optional<ScriptVariableTypeData>& returnValue = func.GetReturnType();

	bool hasReturnValue = returnValue.has_value();
	if (ImGui::Checkbox("Has output", &hasReturnValue))
	{
		if (hasReturnValue)
		{
			func.SetReturnType(ScriptVariableTypeData{ MakeTypeTraits<int>(), "New output" });
		}
		else
		{
			func.SetReturnType(std::nullopt);
		}

		return;
	}

	if (!hasReturnValue)
	{
		return;
	}

	ParamWrapper returnValueWrapped{ *returnValue };

	if (ShowInspectUI("Outputs", returnValueWrapped))
	{
		func.SetReturnType(std::move(returnValueWrapped.mParam));
	}
}

void CE::ScriptEditorSystem::DisplayEventDetails(ScriptFunc& func)
{
	ImGui::TextUnformatted(func.GetName().c_str());
	func.TryGetEvent()->OnDetailsInspect(func);
}

void CE::ScriptEditorSystem::DisplayNodeDetails(ScriptNode& node)
{
	ScriptFunc& currentFunc = *TryGetSelectedFunc();

	ImGui::Text("Selected node %s (%s)", node.GetTitle().c_str(), node.GetSubTitle().c_str());

	if (node.GetType() == ScriptNodeType::Comment)
	{
		CommentScriptNode& asComment = static_cast<CommentScriptNode&>(node);

		std::string comment = asComment.GetComment();

		if (ShowInspectUI("Comment", comment))
		{
			asComment.SetComment(comment);
		}

		LinearColor col = asComment.GetColour();

		if (ShowInspectUI("Colour", col))
		{
			asComment.SetColour(col);
		}

		return;
	}

	for (ScriptPin& inputPin : node.GetInputs(currentFunc))
	{
		if (CanInspectPin(currentFunc, inputPin))
		{
			ImGui::TextUnformatted(inputPin.GetName().c_str());
			ImGui::SameLine();
			InspectPin(currentFunc, inputPin);
		}
	}

	const std::vector<std::reference_wrapper<const ScriptError>> errors = VirtualMachine::Get().GetErrors({ currentFunc, node });

	ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 0.0f, 0.0f, 1.0f });

	for (const ScriptError& error : errors)
	{
		ImGui::TextUnformatted(error.ToString(false).c_str());
	}

	ImGui::PopStyleColor();
}



void CE::ScriptEditorSystem::DisplayMemberDetails(ScriptField& field)
{
	// Reduce code reptition
	std::string memberName = field.GetName();

	if (ShowInspectUI("##memberName", memberName))
	{
		field.SetName(memberName);
	}

	struct FieldTypeFilter
	{
		bool operator()(const MetaType& type) const
		{
			return ScriptField::CanTypeBeUsedForFields(type);
		}
	};

	const MetaType* currentFieldType = field.TryGetType();

	if (currentFieldType == nullptr)
	{
		ImGui::TextUnformatted(Format("{} is no longer a valid type", field.GetTypeName()).c_str());
	}

	MetaTypeFilter<FieldTypeFilter> filter{ currentFieldType };

	if (ShowInspectUI("Type", filter))
	{
		field.SetType(filter == nullptr ? MetaManager::Get().GetType<int32>() : *filter.Get());
	}

	ShowInspectUI("Default value", field.GetDefaultValue());
}

void CE::ParamWrapper::DisplayInspectUI(const std::string&)
{
	std::string paramName = mParam.GetName();
	if (ShowInspectUI("Name", paramName))
	{
		mParam.SetName(paramName);
	}

	struct ReferencedByScriptFilter
	{
		bool operator()(const MetaType& type) const
		{
			return ScriptField::CanTypeBeUsedForFields(type);
		}
	};

	const MetaType* currentParamType = mParam.TryGetType();

	if (currentParamType == nullptr)
	{
		ImGui::TextUnformatted(Format("{} is no longer a valid type", mParam.GetTypeName()).c_str());
	}

	MetaTypeFilter<ReferencedByScriptFilter> filter{ currentParamType };

	if (ShowInspectUI("Type", filter))
	{
		const MetaType& selectedType = filter == nullptr ? MetaManager::Get().GetType<int32>() : *filter.Get();
		mParam = ScriptVariableTypeData{ selectedType, CanTypeBeOwnedByScripts(selectedType) ? TypeForm::Value : TypeForm::Ptr, std::string{ mParam.GetName() }, };
	}

	const MetaType* const type = mParam.TryGetType();

	if (type == nullptr)
	{
		ImGui::TextColored(ImVec4{ 1.0f, 0.0f, 0.0f, 1.0f }, "Type %s doesn't exist anymore, select a different one", mParam.GetTypeName().c_str());
		return;
	}

	if (mParam.GetTypeForm() != TypeForm::Ptr
		&& mParam.GetTypeForm() != TypeForm::Value)
	{
		LOG(LogScripting, Verbose, "Somehow blueprint function accepts a paremeter thats neither a ref or a value. Will be set to reference");
		mParam.SetTypeForm(TypeForm::Value);
	}

	const bool scriptOwnable = CanTypeBeOwnedByScripts(*type);
	bool isPassedByReference = mParam.GetTypeForm() == TypeForm::Ptr;

	bool stylePushed{};
	bool disabled{};

	if (!scriptOwnable)
	{
		if (mParam.GetTypeForm() == TypeForm::Value)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 1.0f, 0.0f, 0.0f, 1.0f });
			ImGui::Text("%s can only be passed by reference", type->GetName().c_str());
			stylePushed = true;
		}
		else
		{
			ImGui::BeginDisabled();
			disabled = true;
		}
	}

	if (ImGui::Checkbox("Pass by reference", &isPassedByReference))
	{
		if (isPassedByReference)
		{
			mParam.SetTypeForm(TypeForm::Ptr);
		}
		else
		{
			mParam.SetTypeForm(TypeForm::Value);
		}
	}

	if (stylePushed)
	{
		ImGui::PopStyleColor();
	}

	if (disabled)
	{
		ImGui::EndDisabled();
	}

	ImGui::SetItemTooltip(
		R"(
Understanding Passing by Reference vs. Passing by Value

Passing by Value:
- Think of it like handing over a photocopy.
- When you pass a value, you're sharing a copy of the original data.
- Changes made to the copy dont affect the original.

Passing by Reference:
- Picture it as giving directions to a specific location.
- When you pass by reference, you're sharing the actual location (memory address) of the data.
- Changes made directly impact the original data.

In a Nutshell:
- Passing by value is like sharing a copy, changes dont affect the original.
- Passing by reference is like sharing the actual thing, changes directly impact the original.

Consideration:
- Use passing by value for simplicity and data integrity.
- Use passing by reference when you want changes to affect the original data directly.)");
}

