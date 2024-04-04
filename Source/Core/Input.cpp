#include "Precomp.h"
#include "Core/Input.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"

CE::MetaType CE::Input::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Input>{}, "Input" };
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](int gamepadID, GamepadAxis axis) { return Input::Get().GetGamepadAxis(gamepadID, axis); }, "GetGamepadAxis", MetaFunc::ExplicitParams<int, GamepadAxis>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](int gamepadID, GamepadButton button) { return Input::Get().IsGamepadButtonHeld(gamepadID, button);}, "IsGamepadButtonHeld", MetaFunc::ExplicitParams<int, GamepadButton>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](int gamepadID, GamepadButton button) { return Input::Get().WasGamepadButtonPressed(gamepadID, button); }, "WasGamepadButtonPressed", MetaFunc::ExplicitParams<int, GamepadButton>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](int gamepadID, GamepadButton button) { return Input::Get().WasGamepadButtonReleased(gamepadID, button); }, "WasGamepadButtonReleased", MetaFunc::ExplicitParams<int, GamepadButton>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([] { return Input::Get().IsMouseAvailable(); }, "IsMouseAvailable").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](MouseButton button) { return Input::Get().IsMouseButtonHeld(button); }, "IsMouseButtonHeld", MetaFunc::ExplicitParams<MouseButton>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](MouseButton button) { return Input::Get().WasMouseButtonPressed(button); }, "WasMouseButtonPressed", MetaFunc::ExplicitParams<MouseButton>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](MouseButton button) { return Input::Get().WasMouseButtonReleased(button); }, "WasMouseButtonReleased", MetaFunc::ExplicitParams<MouseButton>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([] { return Input::Get().GetMousePosition(); }, "GetMousePosition").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([] { return Input::Get().GetMouseWheel(); }, "GetMouseWheel").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([] { return Input::Get().IsKeyboardAvailable(); }, "IsKeyboardAvailable").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](KeyboardKey button) { return Input::Get().IsKeyboardKeyHeld(button); }, "IsKeyboardKeyHeld", MetaFunc::ExplicitParams<KeyboardKey>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](KeyboardKey button) { return Input::Get().WasKeyboardKeyPressed(button); }, "WasKeyboardKeyPressed", MetaFunc::ExplicitParams<KeyboardKey>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](KeyboardKey button) { return Input::Get().WasKeyboardKeyReleased(button); }, "WasKeyboardKeyReleased", MetaFunc::ExplicitParams<KeyboardKey>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](KeyboardKey positive, KeyboardKey negative) { return Input::Get().GetKeyboardAxis(positive, negative); }, "GetKeyboardAxis", MetaFunc::ExplicitParams<KeyboardKey, KeyboardKey>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([] { return Input::Get().HasFocus(); }, "HasFocus").GetProperties().Add(Props::sIsScriptableTag);

	return type;
}

CE::MetaType Reflector<CE::Input::KeyboardKey>::Reflect()
{
	using namespace CE;
	using T = Input::KeyboardKey;
	MetaType type{ MetaType::T<T>{}, "KeyboardKey" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

CE::MetaType Reflector<CE::Input::GamepadAxis>::Reflect()
{
	using namespace CE;
	using T = Input::GamepadAxis;
	MetaType type{ MetaType::T<T>{}, "GamepadAxis" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

CE::MetaType Reflector<CE::Input::GamepadButton>::Reflect()
{
	using namespace CE;
	using T = Input::GamepadButton;
	MetaType type{ MetaType::T<T>{}, "GamepadButton" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

CE::MetaType Reflector<CE::Input::MouseButton>::Reflect()
{
	using namespace CE;
	using T = Input::MouseButton;
	MetaType type{ MetaType::T<T>{}, "MouseButton" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}
