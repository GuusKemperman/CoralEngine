#include "Precomp.h"
#include "Core/Input.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"

CE::MetaType CE::Input::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Input>{}, "Input" };
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](int gamepadID, GamepadAxis axis) { return Input::Get().GetGamepadAxis(gamepadID, axis); }, "GetGamepadAxis").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](int gamepadID, GamepadButton button) { return Input::Get().IsGamepadButtonHeld(gamepadID, button);}, "IsGamepadButtonHeld").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](int gamepadID, GamepadButton button) { return Input::Get().WasGamepadButtonPressed(gamepadID, button); }, "WasGamepadButtonPressed").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](int gamepadID, GamepadButton button) { return Input::Get().WasGamepadButtonReleased(gamepadID, button); }, "WasGamepadButtonReleased").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](int gamepadID) { return Input::Get().IsGamepadAvailable(gamepadID); }, "IsGamepadAvailable").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([] { return Input::Get().IsMouseAvailable(); }, "IsMouseAvailable").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](MouseButton button) { return Input::Get().IsMouseButtonHeld(button); }, "IsMouseButtonHeld").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](MouseButton button) { return Input::Get().WasMouseButtonPressed(button); }, "WasMouseButtonPressed").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](MouseButton button) { return Input::Get().WasMouseButtonReleased(button); }, "WasMouseButtonReleased").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([] { return Input::Get().GetMousePosition(); }, "GetMousePosition").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([] { return Input::Get().GetMouseWheel(); }, "GetMouseWheel").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([] { return Input::Get().IsKeyboardAvailable(); }, "IsKeyboardAvailable").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](KeyboardKey button) { return Input::Get().IsKeyboardKeyHeld(button); }, "IsKeyboardKeyHeld").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](KeyboardKey button) { return Input::Get().WasKeyboardKeyPressed(button); }, "WasKeyboardKeyPressed").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](KeyboardKey button) { return Input::Get().WasKeyboardKeyReleased(button); }, "WasKeyboardKeyReleased").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](KeyboardKey positive, KeyboardKey negative) { return Input::Get().GetKeyboardAxis(positive, negative); }, "GetKeyboardAxis").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([] { return Input::Get().HasFocus(); }, "HasFocus").GetProperties().Add(Props::sIsScriptableTag);

	return type;
}

CE::MetaType Reflector<CE::Input::KeyboardKey>::Reflect()
{
	using namespace CE;
	using T = Input::KeyboardKey;
	MetaType type{ MetaType::T<T>{}, "KeyboardKey" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

CE::MetaType Reflector<CE::Input::GamepadAxis>::Reflect()
{
	using namespace CE;
	using T = Input::GamepadAxis;
	MetaType type{ MetaType::T<T>{}, "GamepadAxis" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

CE::MetaType Reflector<CE::Input::GamepadButton>::Reflect()
{
	using namespace CE;
	using T = Input::GamepadButton;
	MetaType type{ MetaType::T<T>{}, "GamepadButton" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

CE::MetaType Reflector<CE::Input::MouseButton>::Reflect()
{
	using namespace CE;
	using T = Input::MouseButton;
	MetaType type{ MetaType::T<T>{}, "MouseButton" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}
