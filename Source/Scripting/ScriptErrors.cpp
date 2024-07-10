#include "Precomp.h"
#include "Scripting/ScriptErrors.h"

#include "Core/AssetManager.h"
#include "Assets/Script.h"
#include "Scripting/ScriptField.h"
#include "Scripting/ScriptFunc.h"
#include "Scripting/ScriptPin.h"
#include "Scripting/ScriptNode.h"
#include "EditorSystems/AssetEditorSystems/ScriptEditorSystem.h"

CE::ScriptLocation::ScriptLocation(const Script& script) :
	mType(Type::Script),
	mNameOfScript(script.GetName())
{
}

CE::ScriptLocation::ScriptLocation(const ScriptFunc& func) :
	mType(Type::Func),
	mNameOfScript(func.GetNameOfScriptAsset()),
	mFieldOrFuncName(func.GetName())
{
}

CE::ScriptLocation::ScriptLocation(const ScriptField& field) :
	mType(Type::Field),
	mNameOfScript(field.GetNameOfScriptAsset()),
	mFieldOrFuncName(field.GetName())
{
}

CE::ScriptLocation::ScriptLocation(const ScriptFunc& func, const ScriptNode& node) :
	mType(Type::Node),
	mNameOfScript(func.GetNameOfScriptAsset()),
	mFieldOrFuncName(func.GetName()),
	mId(node.GetId())
{
}

CE::ScriptLocation::ScriptLocation(const ScriptFunc& func, const ScriptLink& link) :
	mType(Type::Link),
	mNameOfScript(func.GetNameOfScriptAsset()),
	mFieldOrFuncName(func.GetName()),
	mId(link.GetId())
{
}

CE::ScriptLocation::ScriptLocation(const ScriptFunc& func, const ScriptPin& pin) :
	mType(Type::Pin),
	mNameOfScript(func.GetNameOfScriptAsset()),
	mFieldOrFuncName(func.GetName()),
	mId(std::make_pair(pin.GetId(), pin.GetNodeId()))
{
}

#ifdef EDITOR
void CE::ScriptLocation::NavigateToLocation() const
{
	const std::string systemName = Internal::GetSystemNameBasedOnAssetName(mNameOfScript);
	ScriptEditorSystem* scriptEditor = Editor::Get().TryGetSystem<ScriptEditorSystem>(systemName);

	if (scriptEditor == nullptr)
	{
		WeakAssetHandle<Script> script = AssetManager::Get().TryGetWeakAsset<Script>(mNameOfScript);

		if (script == nullptr)
		{
			LOG(LogEditor, Message, "Could not navigate to script {}, it no longer exists", mNameOfScript);
			return;
		}

		scriptEditor = dynamic_cast<ScriptEditorSystem*>(Editor::Get().TryOpenAssetForEdit(script));

		if (scriptEditor == nullptr)
		{
			LOG(LogEditor, Message, "Could not navigate to script {}, script could not be opened for edit", mNameOfScript);
			return;
		}
	}

	ImGui::SetWindowFocus(systemName.c_str());
	scriptEditor->NavigateTo(*this);
}
#endif // EDITOR

std::string CE::ScriptLocation::ToString() const
{
	return mFieldOrFuncName.empty() ? mNameOfScript : Format("{} ({})", mFieldOrFuncName, mNameOfScript);
}

bool CE::ScriptLocation::IsLocationEqualToOrInsideOf(const ScriptLocation& other) const
{
	if (mNameOfScript != other.mNameOfScript)
	{
		return false;
	}

	switch (other.mType)
	{
		case Type::Script:
		{
			// We already checked if we are in the same script
			return true;
		}
		case Type::Func:
		{
		return mType != Type::Func
			&& mType != Type::Script
			&& mFieldOrFuncName == other.mFieldOrFuncName;
		}
		case Type::Field:
		{
			return mType == Type::Field
				&& mFieldOrFuncName == other.mFieldOrFuncName;
		}
		case Type::Node:
		{
			if (mFieldOrFuncName != other.mFieldOrFuncName)
			{
				return false;
			}

			NodeId otherNodeId = std::get<NodeId>(other.mId);

			if (mType == Type::Node)
			{
				return std::get<NodeId>(mId) == otherNodeId;
			}

			if (mType == Type::Pin)
			{
				const auto& [pinId, nodeId] = std::get<std::pair<PinId, NodeId>>(mId);
				return nodeId == otherNodeId;
			}
			return false;
		}
		case Type::Pin:
		{
			return mType == Type::Pin
				&& std::get<std::pair<PinId, NodeId>>(mId) == std::get<std::pair<PinId, NodeId>>(other.mId)
				&& mFieldOrFuncName == other.mFieldOrFuncName;
		}
		case Type::Link:
		{
			return mType == Type::Link
				&& std::get<LinkId>(mId) == std::get<LinkId>(other.mId)
				&& mFieldOrFuncName == other.mFieldOrFuncName;
		}
	}
	ABORT;
	return false;
}

std::string CE::ScriptError::ToString(bool includeLocationInString) const
{
	std::string str = includeLocationInString ? Format("{} - {}", mOrigin.ToString(), EnumToString(mType)) : std::string{ EnumToString(mType) };;

	if (mAdditionalInfo.has_value())
	{
		str += Format("- {}", *mAdditionalInfo);
	}
	return str;
}
