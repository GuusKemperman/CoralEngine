#pragma once
#include "Meta/MetaTypeTraits.h"
#include "Scripting/ScriptConfig.h"

namespace Engine
{
	class MetaFunc;
	class MetaType;
	class MetaField;
	class ScriptPin;
	class ScriptFunc;
	class Script;

	struct TypeTraits;

	bool CanFunctionBeTurnedIntoNode(const MetaFunc& func);
	
	bool IsFunctionPure(const MetaFunc& func);

	bool WasTypeCreatedByScript(const MetaType& type);

	std::shared_ptr<const Script> TryGetScriptResponsibleForCreatingType(const MetaType& type);

	// Ignores any properties associated with the function, a function may not have the scriptable keyword,
	// but this function can obviously not check that and will return true/false only based on the constness of its parameters
	bool IsFunctionPure(const std::vector<TypeTraits>& functionParameters, TypeTraits returnValue);

	/*
	Checks if a type has the Props::sIsScriptableTag property.
	*/
	bool CanTypeBeReferencedInScripts(const MetaType& type);

	/*
	Checks if a type has the Props::sIsScriptableTag and Props::sIsScriptOwnableTag property. If a type can be owned by scripts, scripts can interact
	with functions that take such a type by value:
	
	Example:
		// Since glm::vec3 is Props::sIsScriptOwnableTag, this function can be called from scripts.
		
		void DoThing(glm::vec3 notAConstRef);

		// Since TransformComponent is not Props::sIsScriptOwnableTag, this function cannot be called 
		// from scripts and will throw an error.
		
		void DoThing(TransformComponent notAConstRef);
	*/
	bool CanTypeBeOwnedByScripts(const MetaType& type);

	bool CanTypeBeUsedInScripts(const MetaType& type, TypeForm inForm);

	bool CanBeSetThroughScripts(const MetaField& field);

	bool CanBeGetThroughScripts(const MetaField& field);

	bool CanCreateLink(const ScriptPin& a, const ScriptPin& b);

	// If false, a value is allowed to be default constructed to pass to the pin
	bool DoesPinRequireLink(const ScriptFunc& inFunc, const ScriptPin& pin);

#ifdef EDITOR
	bool CanInspectPin(const ScriptFunc& func, const ScriptPin& pin);

	void InspectPin(const ScriptFunc& func, ScriptPin& pin);

#else
	CONSTEVAL bool CanInspectPin(const ScriptFunc&, const ScriptPin&) { return false; }
#endif // EDITOR
}