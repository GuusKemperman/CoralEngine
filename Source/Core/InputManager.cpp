#include "Precomp.h"
#include "Core/InputManager.h"

#include <GLFW/glfw3.h>
#include "imgui/imgui_internal.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

void Engine::InputManager::NewFrame()
{
	glfwPollEvents();
}

glm::vec2 Engine::InputManager::GetMousePos()
{
	return ImGui::GetMousePos();
}

bool Engine::InputManager::IsKeyPressed(const ImGuiKey key, const bool checkFocus)
{
	if (checkFocus && !HasFocus())
	{
		return false;
	}

	return ImGui::IsKeyPressed(key);
}

bool Engine::InputManager::IsKeyDown(const ImGuiKey key, const bool checkFocus)
{
	if (checkFocus && !HasFocus())
	{
		return false;
	}

	return ImGui::IsKeyDown(key);
}

bool Engine::InputManager::IsKeyReleased(const ImGuiKey key, const bool checkFocus)
{
	if (checkFocus && !HasFocus())
	{
		return false;
	}

	return ImGui::IsKeyReleased(key);
}

bool Engine::InputManager::HasFocus()
{
#ifdef EDITOR
	return ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
#else
	// No editor windows, so assume we always have focus.
	return true;
#endif // EDITOR
}

Engine::MetaType Engine::InputManager::Reflect()
{
	MetaType type = MetaType{MetaType::T<InputManager>{}, "InputManager" };
	type.AddFunc(&InputManager::GetMousePos, "GetMousePos").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::bind(&InputManager::IsKeyPressed, std::placeholders::_1, true), "IsKeyPressed", MetaFunc::ExplicitParams<ImGuiKey>{}, "key").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::bind(&InputManager::IsKeyDown, std::placeholders::_1, true), "IsKeyDown", MetaFunc::ExplicitParams<ImGuiKey>{}, "key").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::bind(&InputManager::IsKeyReleased, std::placeholders::_1, true), "IsKeyReleased", MetaFunc::ExplicitParams<ImGuiKey>{}, "key").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::bind(&InputManager::GetAxis, std::placeholders::_1, std::placeholders::_2, true), "GetAxis", MetaFunc::ExplicitParams<ImGuiKey, ImGuiKey>{}, "positive", "negative").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&InputManager::GetScrollY, "GetScrollY", "checkFocus").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&InputManager::HasFocus, "HasFocus").GetProperties().Add(Props::sIsScriptableTag);
	return type;
}

float Engine::InputManager::GetAxis(const ImGuiKey positive, const ImGuiKey negative, const bool checkFocus)
{
	return checkFocus && !HasFocus() ? 0.0f : ImGui::GetKeyData(positive)->AnalogValue - ImGui::GetKeyData(negative)->AnalogValue;
}

float Engine::InputManager::GetScrollY(const bool checkFocus)
{
	if ((checkFocus 
		&& !HasFocus())
		|| ImGui::GetCurrentWindowRead() == nullptr)
	{
		return 0.0f;
	}

	return ImGui::GetScrollY();
}
