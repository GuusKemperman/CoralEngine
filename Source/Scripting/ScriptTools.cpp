#include "Precomp.h"
#include "Scripting/ScriptTools.h"

#include "Meta/MetaFunc.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"
#include "Meta/MetaType.h"
#include "Utilities/Imgui/ImguiInspect.h"
#include "Scripting/ScriptPin.h"
#include "Scripting/ScriptFunc.h"
#include "Assets/Script.h"
#include "Core/AssetManager.h"
#include "Scripting/ScriptNode.h"

bool Engine::CanFunctionBeTurnedIntoNode(const MetaFunc& func)
{
	if (!func.GetProperties().Has(Props::sIsScriptableTag))
	{
		return false;
	}

	MetaManager& manager = MetaManager::Get();

	if (func.GetReturnType().mTypeTraits.mStrippedTypeId != MakeTypeId<void>())
	{
		const MetaType* const returnType = manager.TryGetType(func.GetReturnType().mTypeTraits.mStrippedTypeId);

		if (returnType == nullptr
			|| !CanTypeBeUsedInScripts(*returnType, func.GetReturnType().mTypeTraits.mForm))
		{
			LOG_FMT(LogScripting, Warning, "Function {} was marked as scriptable, but the return value was either not reflected or not scriptable",
				func.GetDesignerFriendlyName());
			return false;
		}
	}

	for (const MetaFuncNamedParam& param : func.GetParameters())
	{
		const MetaType* const paramType = manager.TryGetType(param.mTypeTraits.mStrippedTypeId);

		if (paramType == nullptr
			|| !CanTypeBeUsedInScripts(*paramType, param.mTypeTraits.mForm))
		{
			LOG_FMT(LogScripting, Warning, "Function {} was marked as scriptable, but parameter {} was either not reflected or not scriptable",
				func.GetDesignerFriendlyName(),
				param.mName);
			return false;
		}
	}

	return true;
}

bool Engine::IsFunctionPure(const MetaFunc& func)
{
	const std::optional<bool> isPure = func.GetProperties().TryGetValue<bool>(Props::sIsScriptPure);

	if (isPure.has_value())
	{
		return *isPure;
	}

	return IsFunctionPure(std::vector<TypeTraits>{ func.GetParameters().data(), 
		func.GetParameters().data() + func.GetParameters().size() },
		func.GetReturnType());
}

bool Engine::WasTypeCreatedByScript(const MetaType& type)
{
	return type.GetProperties().Has(Name{ Script::sWasTypeCreatedFromScriptProperty });
}

std::shared_ptr<const Engine::Script> Engine::TryGetScriptResponsibleForCreatingType(const MetaType& type)
{
	if (!WasTypeCreatedByScript(type))
	{
		return nullptr;
	}

	return AssetManager::Get().TryGetAsset<Script>(type.GetName());
}

bool Engine::IsFunctionPure(const std::vector<TypeTraits>& functionParameters, const TypeTraits returnValue)
{
	if (returnValue.mStrippedTypeId == MakeTypeId<void>())
	{
		return false;
	}

	for (const TypeTraits paramTraits : functionParameters)
	{
		switch (paramTraits.mForm)
		{
		case TypeForm::Ref:
		case TypeForm::Ptr:
		case TypeForm::RValue:
			return false;
		case TypeForm::Value:
		case TypeForm::ConstRef:
		case TypeForm::ConstPtr:
		{}
		}
	}

	return true;
}

bool Engine::CanTypeBeReferencedInScripts(const MetaType& type)
{
	return type.GetProperties().Has(Props::sIsScriptableTag);
}

bool Engine::CanTypeBeOwnedByScripts(const MetaType& type)
{
	const bool hasTags = type.GetProperties().Has(Props::sIsScriptableTag) && type.GetProperties().Has(Props::sIsScriptOwnableTag);

	if (!hasTags)
	{
		return false;
	}

	const bool hasFunctionsNeeded = type.IsDefaultConstructible()
		&& type.IsMoveConstructible()
		&& type.IsCopyConstructible()
		&& type.IsCopyAssignable()
		&& type.IsMoveAssignable();

	if (!hasFunctionsNeeded)
	{
		LOG_FMT(LogScripting, Warning, "Type {} was marked as script_ownable, but was missing some of the required functions. See surrounding code.", 
			type.GetName())
	}

	return hasTags && hasFunctionsNeeded;
}

bool Engine::CanTypeBeUsedInScripts(const MetaType& type, const TypeForm inForm)
{
	return inForm == TypeForm::Value ? CanTypeBeOwnedByScripts(type) : CanTypeBeReferencedInScripts(type);
}

namespace Engine
{
	static bool CanMemberBeSetOrGetThroughScripts(const MetaField& field)
	{
		const bool hasProperty = field.GetProperties().Has(Props::sIsScriptableTag);

		if (!hasProperty)
		{
			return false;
		}

		return CanTypeBeUsedInScripts(field.GetType(), TypeForm::Ref);
	}
}

bool Engine::CanBeSetThroughScripts(const MetaField& field)
{
	return CanMemberBeSetOrGetThroughScripts(field);
}

bool Engine::CanBeGetThroughScripts(const MetaField& field)
{
	return CanMemberBeSetOrGetThroughScripts(field);
}

bool Engine::CanCreateLink(const ScriptPin& a, const ScriptPin& b)
{
	if (a.GetKind() == b.GetKind()
		|| a.GetNodeId() == b.GetNodeId())
	{
		return false;
	}

	if (a.IsFlow() || b.IsFlow())
	{
		return a.IsFlow() == b.IsFlow();
	}

	if (a.TryGetType() == nullptr 
		|| b.TryGetType() == nullptr)
	{
		return false;
	}

	if (a.TryGetType() == b.TryGetType())
	{
		return true;
	}

	const ScriptPin& input = a.IsInput() ? a : b;
	const ScriptPin& output = a.IsOutput() ? a : b;

	return !MetaFunc::CanArgBePassedIntoParam({ output.TryGetType()->GetTypeId(), output.GetTypeForm() }, { input.TryGetType()->GetTypeId(), input.GetTypeForm() }).has_value();
}

bool Engine::DoesPinRequireLink(const ScriptFunc& func, const ScriptPin& pin)
{
	if (pin.GetTypeForm() == TypeForm::Value
		|| pin.GetTypeForm() == TypeForm::ConstRef
		|| pin.GetTypeForm() == TypeForm::ConstPtr)
	{
		return func.GetNode(pin.GetNodeId()).GetType() == ScriptNodeType::Rerout;
	}
	return true;
}

#ifdef EDITOR

bool Engine::CanInspectPin(const ScriptFunc& func, const ScriptPin& pin)
{
	if (pin.IsInput()
		&& !pin.IsLinked()
		&& !DoesPinRequireLink(func, pin))
	{
		const MetaType* const type = pin.TryGetType();

		if (type != nullptr
			&& type->IsDefaultConstructible()
			&& CanBeInspected(*type))
		{
			return true;
		}
	}
	return false;
}

void Engine::InspectPin(const ScriptFunc& func, ScriptPin& pin)
{
	if (!CanInspectPin(func, pin)) 
	{
		UNLIKELY;
		LOG_FMT(LogEditor, Warning, "Cannot inspect pin {}. You can check using CanInspectPin", pin.GetName());
		return;
	}

	const std::string invisPinName = Format("##PIN{})", pin.GetName());

	MetaAny* toInspect = pin.TryGetValueIfNoInputLinked();

	if (toInspect != nullptr)
	{
		ShowInspectUI(invisPinName, *toInspect);
	}
	else
	{
		const MetaType* const type = pin.TryGetType();
		ASSERT(type != nullptr);

		FuncResult defaultConstructed = type->Construct();

		if (defaultConstructed.HasError())
		{
			LOG_FMT(LogEditor, Warning, "Cannot inspect pin {}, as it could not be default constructed - {}", pin.GetName(), defaultConstructed.Error());
			return;
		}

		if (ShowInspectUI(invisPinName, defaultConstructed.GetReturnValue()))
		{
			pin.SetValueIfNoInputLinked(std::move(defaultConstructed.GetReturnValue()));
		}
	}
}

#endif // EDITOR