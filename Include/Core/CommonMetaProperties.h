#pragma once

namespace Engine::Props
{
	/*
	Use on:
		Classes

	Description:
		Mark a class as a component. The component will show up in the dropdown field
		when attempting to add a component through the editor.
		The function Registry::AddComponent(const MetaType& type, entt::entity) can only
		be called for Props::sComponentTag types.

	Example:
		type.GetProperties().Add(Props::sComponentTag)
	*/
	static constexpr std::string_view sComponentTag = "sComponentTag";

	/*
	Use on:
		Classes

	Description:
		Set the class version. Can be used for assets that were saved in an old format.
		See AssetLoadInfo::GetVersion()

	Example:
		type.GetProperties().Set(Props::sVersion, 5)
	*/
	static constexpr std::string_view sVersion = "sVersion";

	/*
	Use on:
		Classes, Functions, Fields

	Description:
		Mark a class, function or field as one that can be interacted with through visual scripts.

		For classes, this means a reference to an instance of that class can be used in scripts. If you want to a script to use
		a value of that class, see Props::sIsScriptOwnableTag. It is perfectly valid to have a Props::sIsScriptableTag function in a class that is not Props::sIsScriptableTag

	Example:
		type.GetProperties().Add(Props::sIsScriptableTag)
	*/
	static constexpr std::string_view sIsScriptableTag = "sIsScriptableTag";

	/*
	Use on:
		Classes

	Description:
		Only types that do NOT have the Props::sIsScriptOwnableTag property cannot be 'copied' or owned by a script.
		This means parameters need to be by reference, and that a type that is not Props::sIsScriptOwnableTag cannot
		be a field of a script.
		This prevents designers from holding onto components for example, since pointer stability does
		not exist.

	Example:
		type.GetProperties().Add(Props::sIsScriptOwnableTag)
	*/
	static constexpr std::string_view sIsScriptOwnableTag = "sIsScriptOwnableTag";

	/*
	Use on:
		Functions

	Description:
		By default, functions that take mutable references are considered impure.
		This property can be used to override this.

	Example:
		func.GetProperties().Set(Props::sIsScriptPure, true);
		func.GetProperties().Set(Props::sIsScriptPure, false);
	*/
	static constexpr std::string_view sIsScriptPure = "sIsScriptPure";

	/*
	Use on:
		Fields

	Description:
		By default, each field is serialized and deserialized.
		This property can be used to disable this behaviour.

	Example:
		field.GetProperties().Add(Props::sNoSerializeTag);
	*/
	static constexpr std::string_view sNoSerializeTag = "sNoSerializeTag";

	/*
	Use on:
		Fields, Type

	Description:
		By default, each field can be inspected through the editor.

		By adding this property to a field, the field is hidden from the user.

		By adding this property to a type, each field is hidden from the user.
		If this type is a component, it's existence is hidden from the user,
		it cannot be added through the UI.

		For components, this property hides all the fields

	Example:
		field.GetProperties().Add(Props::sNoInspectTag);
	*/
	static constexpr std::string_view sNoInspectTag = "sNoInspectTag";

	/*
	Use on:
		Functions (as long as they don't have parameters)

	Description:
		Adds a button in the inspector. Clicking it calls the function.

	Example:
		func.GetProperties().Add(Props::sCallFromEditorTag);
	*/
	static constexpr std::string_view sCallFromEditorTag = "sCallFromEditorTag";

	/*
	Use on:
		Fields

	Description:
		Makes the variable read-only, it can not be modified through the editor.

		This is implemented by disabling all interaction with the widget. This
		means this may not work for more complex widgets, such as vectors, as
		the user also won't be able to open the collapsing header to view the
		vector.

	Example:
		field.GetProperties().Add(Props::sIsEditorReadOnlyTag);
	*/
	static constexpr std::string_view sIsEditorReadOnlyTag = "sIsEditorReadOnlyTag";

	/*
	Use on:
		Fields

	Description:
		Makes the variable read-only, it can not be modified through scripts.

		The variable can still be gotten through scripts.

	Example:
		field.GetProperties().Add(Props::sIsScriptReadOnlyTag);
	*/
	static constexpr std::string_view sIsScriptReadOnlyTag = "sIsScriptReadOnlyTag";

	// If a type has a property with this name, it was created from a script

	/*
	Use on:
		Types, Functions, Fields

	Description:
		Can be used to find out whether a type, function or field originates from a
		reflected C++ equivalent, or if it was created at runtime based on a visual
		script. Each type, function and field created based on a script will have
		this tag.
	*/
	static constexpr std::string_view sIsFromScriptsTag = "sIsFromScriptsTag";

	/*
	Use on:
		Classes deriving from Engine::EditorSystem. The class must be default constructible.

	Description:
		Automatically adds the system the VERY FIRST time the engine starts.

		If the Editor has not saved which systems are active and which aren't, all systems
		with this property are created. This is saved in the Intermediate directory. But in
		case, if your system is default constructible, the system will still show up in the
		View menu at the top of the menubar.

	Example:
		type.GetProperties().Add(Props::sEditorSystemDefaultOpenTag);
	*/
	static constexpr std::string_view sEditorSystemDefaultOpenTag = "sEditorSystemDefaultOpenTag";
}