#include "Precomp.h"
#include "Core/Input.h"
#include "Core/Device.h"

using namespace CE;

enum KeyAction
{
    Release = 0,
    Press = 1,
    None = 2
};

namespace
{
    constexpr int nr_keys = 350;

    bool keys_down[nr_keys];
    bool prev_keys_down[nr_keys];
    KeyAction keys_action[nr_keys];

    constexpr int nr_mousebuttons = 8;
    bool mousebuttons_down[nr_mousebuttons];
    bool prev_mousebuttons_down[nr_mousebuttons];
    KeyAction mousebuttons_action[nr_mousebuttons];

    constexpr int max_nr_gamepads = 4;
    bool gamepad_connected[max_nr_gamepads];
    GLFWgamepadstate gamepad_state[max_nr_gamepads];
    GLFWgamepadstate prev_gamepad_state[max_nr_gamepads];

    glm::vec2 mousepos;
    glm::vec2 previousmousepos;
    float mousewheelDelta = 0;

    void cursor_position_callback(GLFWwindow*, double xpos, double ypos)
    {
        mousepos.x = (float)xpos;
        mousepos.y = (float)ypos;
    }

    void scroll_callback(GLFWwindow*, double, double yoffset) { mousewheelDelta = (float)yoffset; }

    void key_callback(GLFWwindow*, int key, int, int action, int)
    {
        if (action == GLFW_PRESS || action == GLFW_RELEASE) keys_action[key] = static_cast<KeyAction>(action);
    }

    void mousebutton_callback(GLFWwindow*, int button, int action, int)
    {
        if (action == GLFW_PRESS || action == GLFW_RELEASE) mousebuttons_action[button] = static_cast<KeyAction>(action);
    }
}

Input::Input()
{
    if (Device::IsHeadless())
    {
        return;
    }

    GLFWwindow* window = reinterpret_cast<GLFWwindow*>(Device::Get().GetWindow());

    // glfwSetJoystickCallback(joystick_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mousebutton_callback);
    glfwSetScrollCallback(window, scroll_callback);
}

Input::~Input()
{
    if (Device::IsHeadless())
    {
        return;
    }

    GLFWwindow* window = reinterpret_cast<GLFWwindow*>(Device::Get().GetWindow());
    
    // glfwSetJoystickCallback(NULL);
    glfwSetCursorPosCallback(window, NULL);
}

void Input::NewFrame()
{
    previousmousepos = mousepos;
    glfwPollEvents();

    // update keyboard key states
    for (int i = 0; i < nr_keys; ++i)
    {
        prev_keys_down[i] = keys_down[i];

        if (keys_action[i] == KeyAction::Press)
            keys_down[i] = true;
        else if (keys_action[i] == KeyAction::Release)
            keys_down[i] = false;

        keys_action[i] = KeyAction::None;
    }

    // update mouse button states
    for (int i = 0; i < nr_mousebuttons; ++i)
    {
        prev_mousebuttons_down[i] = mousebuttons_down[i];

        if (mousebuttons_action[i] == KeyAction::Press)
            mousebuttons_down[i] = true;
        else if (mousebuttons_action[i] == KeyAction::Release)
            mousebuttons_down[i] = false;

        mousebuttons_action[i] = KeyAction::None;
    }

    // update gamepad states
    for (int i = 0; i < max_nr_gamepads; ++i)
    {
        prev_gamepad_state[i] = gamepad_state[i];

        if (glfwJoystickPresent(i) && glfwJoystickIsGamepad(i))
            gamepad_connected[i] = static_cast<bool>(glfwGetGamepadState(i, &gamepad_state[i]));
    }
}

bool Input::IsGamepadAvailable(int gamepadID) const
{
	return gamepad_connected[gamepadID];
}

float Input::GetGamepadAxis(int gamepadID, GamepadAxis axis, bool checkFocus) const
{
    if (!IsGamepadAvailable(gamepadID)
        || !HasFocus(checkFocus))
    {
        return 0.0;
    }

    int a = static_cast<int>(axis);
    ASSERT(a >= 0 && a <= GLFW_GAMEPAD_AXIS_LAST);

    float result = gamepad_state[gamepadID].axes[a];
    if (axis == GamepadAxis::TriggerLeft || axis == GamepadAxis::TriggerRight)
    {
        result = result * 0.5f + 0.5f;
    }

    return result;
}

bool Input::IsGamepadButtonHeld(int gamepadID, GamepadButton button, bool checkFocus) const
{
    if (!IsGamepadAvailable(gamepadID)
        || !HasFocus(checkFocus))
    {
        return false;
    }
    int b = static_cast<int>(button);
    ASSERT(b >= 0 && b <= GLFW_GAMEPAD_BUTTON_LAST);
    return static_cast<bool>(gamepad_state[gamepadID].buttons[b]);
}

bool Input::WasGamepadButtonPressed(int gamepadID, GamepadButton button, bool checkFocus) const
{
    if (!IsGamepadAvailable(gamepadID)
        || !HasFocus(checkFocus))
    {
        return false;
    }

    if (button == GamepadButton::TriggerRight)
    {
        return GetGamepadAxis(gamepadID, GamepadAxis::TriggerRight, checkFocus) > sTriggerThreshold;
    }
    if (button == GamepadButton::TriggerLeft)
    {
        return GetGamepadAxis(gamepadID, GamepadAxis::TriggerLeft, checkFocus) > sTriggerThreshold;
    }

    int b = static_cast<int>(button);

    ASSERT(b >= 0 && b <= GLFW_GAMEPAD_BUTTON_LAST);
    return !static_cast<bool>(prev_gamepad_state[gamepadID].buttons[b]) &&
        static_cast<bool>(gamepad_state[gamepadID].buttons[b]);
}

bool Input::WasGamepadButtonReleased(int gamepadID, GamepadButton button, bool checkFocus) const
{
    if (!IsGamepadAvailable(gamepadID)
        || !HasFocus(checkFocus))
    {
        return false;
    }

    int b = static_cast<int>(button);

    ASSERT(b >= 0 && b <= GLFW_GAMEPAD_BUTTON_LAST);
    return static_cast<bool>(prev_gamepad_state[gamepadID].buttons[b]) &&
        !static_cast<bool>(gamepad_state[gamepadID].buttons[b]);
}

bool Input::IsMouseAvailable() const { return true; }

bool Input::IsMouseButtonHeld(MouseButton button, bool checkFocus) const
{
    if (!HasFocus(checkFocus))
    {
        return false;
    }

    int b = static_cast<int>(button);
    return mousebuttons_down[b];
}

bool Input::WasMouseButtonPressed(MouseButton button, bool checkFocus) const
{
    if (!HasFocus(checkFocus))
    {
        return false;
    }

    int b = static_cast<int>(button);
    return mousebuttons_down[b] && !prev_mousebuttons_down[b];
}

bool Input::WasMouseButtonReleased(MouseButton button, bool checkFocus) const
{
    if (!HasFocus(checkFocus))
    {
        return false;
    }

    int b = static_cast<int>(button);
    return !mousebuttons_down[b] && prev_mousebuttons_down[b];
}

glm::vec2 Input::GetMousePosition() const { return mousepos; }

glm::vec2 Input::GetDeltaMousePosition() const { return mousepos - previousmousepos; }

float Input::GetMouseWheel(bool checkFocus) const
{
    if (!HasFocus(checkFocus))
    {
        return 0.0f;
    }

	return mousewheelDelta;
}

bool Input::IsKeyboardAvailable() const { return true; }

bool Input::IsKeyboardKeyHeld(KeyboardKey key, bool checkFocus) const
{
    if (!HasFocus(checkFocus))
    {
        return false;
    }

    int k = static_cast<int>(key);
    ASSERT(k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST);
    return keys_down[k];
}

bool Input::WasKeyboardKeyPressed(KeyboardKey key, bool checkFocus) const
{
    if (!HasFocus(checkFocus))
    {
        return false;
    }

    int k = static_cast<int>(key);
    ASSERT(k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST);
    return keys_down[k] && !prev_keys_down[k];
}

bool Input::WasKeyboardKeyReleased(KeyboardKey key, bool checkFocus) const
{
    if (!HasFocus(checkFocus))
    {
        return false;
    }

    int k = static_cast<int>(key);
    ASSERT(k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST);
    return !keys_down[k] && prev_keys_down[k];
}

float Input::GetKeyboardAxis(KeyboardKey positive, KeyboardKey negative, bool checkFocus) const
{
    if (!HasFocus(checkFocus))
    {
        return 0.0f;
    }

    float axis = IsKeyboardKeyHeld(positive, false) ? 1.0f : 0.0f;

	if (IsKeyboardKeyHeld(negative, false))
    {
        axis -= 1.0f;
    }

    return axis;
}

bool Input::HasFocus() const
{
#ifdef EDITOR
    return !Device::IsHeadless()  // ImGui may not have been initialised
		&& ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
#else
    // No editor windows, so assume we always have focus.
    return true;
#endif // EDITOR
}

bool Input::HasFocus(bool checkFocus) const
{
    if (!checkFocus)
    {
        return true;
    }
    return HasFocus();
}