#pragma once
#include "magic_enum/magic_enum.hpp"

#include "Meta/MetaReflect.h"
#include "Core/EngineSubsystem.h"

namespace CE
{
    /// <summary>
	/// The Input class provides a cross-platform way to handle input from keyboard, mouse, and gamepads.
	/// </summary>
    class Input final :
		public EngineSubsystem<Input>
    {
        friend EngineSubsystem;
        friend MetaType;
        Input();
    	~Input();

    public:
	    void NewFrame();

    	/// <summary>
        /// An enum listing all supported keyboard keys.
        /// This uses the same numbering as in GLFW input, so a GLFW-based implementation can use it directly without any further
        /// mapping.
        /// </summary>
        enum class KeyboardKey
        {
            Space = 32,
            Apostrophe = 39,
            Comma = 44,
            Minus = 45,
            Period = 46,
            Slash = 47,
            Digit0 = 48,
            Digit1 = 49,
            Digit2 = 50,
            Digit3 = 51,
            Digit4 = 52,
            Digit5 = 53,
            Digit6 = 54,
            Digit7 = 55,
            Digit8 = 56,
            Digit9 = 57,
            Semicolon = 59,
            Equal = 61,
            A = 65,
            B = 66,
            C = 67,
            D = 68,
            E = 69,
            F = 70,
            G = 71,
            H = 72,
            I = 73,
            J = 74,
            K = 75,
            L = 76,
            M = 77,
            N = 78,
            O = 79,
            P = 80,
            Q = 81,
            R = 82,
            S = 83,
            T = 84,
            U = 85,
            V = 86,
            W = 87,
            X = 88,
            Y = 89,
            Z = 90,
            LeftBracket = 91,
            Backslash = 92,
            RightBracket = 93,
            GraveAccent = 96,
            World1 = 161,
            World2 = 162,
            Escape = 256,
            Enter = 257,
            Tab = 258,
            Backspace = 259,
            Insert = 260,
            Delete = 261,
            ArrowRight = 262,
            ArrowLeft = 263,
            ArrowDown = 264,
            ArrowUp = 265,
            PageUp = 266,
            PageDown = 267,
            Home = 268,
            End = 269,
            CapsLock = 280,
            ScrollLock = 281,
            NumLock = 282,
            PrintScreen = 283,
            Pause = 284,
            F1 = 290,
            F2 = 291,
            F3 = 292,
            F4 = 293,
            F5 = 294,
            F6 = 295,
            F7 = 296,
            F8 = 297,
            F9 = 298,
            F10 = 299,
            F11 = 300,
            F12 = 301,
            F13 = 302,
            F14 = 303,
            F15 = 304,
            F16 = 305,
            F17 = 306,
            F18 = 307,
            F19 = 308,
            F20 = 309,
            F21 = 310,
            F22 = 311,
            F23 = 312,
            F24 = 313,
            F25 = 314,
            Numpad0 = 320,
            Numpad1 = 321,
            Numpad2 = 322,
            Numpad3 = 323,
            Numpad4 = 324,
            Numpad5 = 325,
            Numpad6 = 326,
            Numpad7 = 327,
            Numpad8 = 328,
            Numpad9 = 329,
            NumpadDecimal = 330,
            NumpadDivide = 331,
            NumpadMultiply = 332,
            NumpadSubtract = 333,
            NumpadAdd = 334,
            NumpadEnter = 335,
            NumpadEqual = 336,
            LeftShift = 340,
            LeftControl = 341,
            LeftAlt = 342,
            LeftSuper = 343,
            RightShift = 344,
            RightControl = 345,
            RightAlt = 346,
            RightSuper = 347,
            Menu = 348
        };

    	/// <summary>
        /// An enum listing all supported gamepad axes with analog input values.
        /// This uses the same numbering as in GLFW input, so a GLFW-based implementation can use it directly without any further
        /// mapping.
        /// </summary>
        enum class GamepadAxis
        {
            /// Represents the horizontal axis of the left gamepad stick, with an analog input value between -1 (left) and 1
            /// (right).
            StickLeftX = 0,
            /// Represents the vertical axis of the left gamepad stick, with an analog input value between -1 (down) and 1 (up).
            StickLeftY = 1,
            /// Represents the horizontal axis of the right gamepad stick, with an analog input value between -1 (left) and 1
            /// (right).
            StickRightX = 2,
            /// Represents the vertical axis of the right gamepad stick, with an analog input value between -1 (down) and 1 (up).
            StickRightY = 3,
            /// Represents the left trigger of a gamepad, with an analog input value between 0 (not pressed) and 1 (fully pressed).
            TriggerLeft = 4,
            /// Represents the right trigger of a gamepad, with an analog input value between 0 (not pressed) and 1 (fully pressed).
            TriggerRight = 5
        };

        /// <summary>
        /// An enum listing all supported gamepad buttons with digital input values.
        /// This uses the same numbering as in GLFW input, so a GLFW-based implementation can use it directly without any further
        /// mapping.
        /// </summary>
        enum class GamepadButton
        {
            /// Represents the bottom (south) button of the 4 main action buttons on a gamepad.
            South = 0,
            /// Represents the right (east) button of the 4 main action buttons on a gamepad.
            East = 1,
            /// Represents the left (west) button of the 4 main action buttons on a gamepad.
            West = 2,
            /// Represents the top (north) button of the 4 main action buttons on a gamepad.
            North = 3,

            /// Represents the left shoulder button on a gamepad.
            ShoulderLeft = 4,
            /// Represents the right shoulder button on a gamepad.
            ShoulderRight = 5,

            /// Represents the left of the two menu-related buttons on a gamepad.
            /// This button has different names on different platforms, such as Share (Xbox) or - (Switch). The PS5 does not have
            /// such a button.
            MenuLeft = 6,
            /// Represents the right of the two menu-related buttons on a gamepad.
            /// This button has different names on different platforms, such as Menu (Xbox), Start (PS5), or + (Switch).
            MenuRight = 7,

            // Button 8 is not used, so that we have a 1-on-1 mapping with the GLFW enum.

            /// Represents the pressing of the left gamepad stick.
            StickPressLeft = 9,
            /// Represents the pressing of the right gamepad stick.
            StickPressRight = 10,

            /// Represents the up arrow of the D-pad on a gamepad.
            DPadUp = 11,
            /// Represents the right arrow of the D-pad on a gamepad.
            DPadRight = 12,
            /// Represents the down arrow of the D-pad on a gamepad.
            DPadDown = 13,
            /// Represents the left arrow of the D-pad on a gamepad.
            DPadLeft = 14,

            /// Represents the left trigger of a gamepad, with an analog input value between 0 (not pressed) and 1 (fully pressed).
            /// To turn it into a button we just take the middle value as a threshold.
            TriggerLeft = 15,
            /// Represents the right trigger of a gamepad, with an analog input value between 0 (not pressed) and 1 (fully pressed).
            /// To turn it into a button we just take the middle value as a threshold.
        	TriggerRight = 16
        };

        /// <summary>
        /// An enum listing all supported mouse buttons.
        /// This uses the same numbering as in GLFW input, so a GLFW-based implementation can use it directly without any further
        /// mapping.
        /// </summary>
        enum class MouseButton
        {
            Left = 0,
            Right = 1,
            Middle = 2
        };

        /// Returns the current floating-point value of a given gamepad axis.
        float GetGamepadAxis(int gamepadID, GamepadAxis axis, bool checkFocus = true) const;

        /// Checks and returns whether a gamepad with a given ID is currently available for input.
        bool IsGamepadAvailable(int gamepadID) const;

        /// Checks and returns whether a given gamepad button is being held down in the current frame.
        bool IsGamepadButtonHeld(int gamepadID, GamepadButton button, bool checkFocus = true) const;

        /// Checks and returns whether a given gamepad button is being pressed in the current frame without having been pressed in
        /// the previous frame.
        bool WasGamepadButtonPressed(int gamepadID, GamepadButton button, bool checkFocus = true) const;

        /// Checks and returns whether a given gamepad button is being pressed in the current frame without having been pressed in
		/// the previous frame.
        bool WasGamepadButtonReleased(int gamepadID, GamepadButton button, bool checkFocus = true) const;

        /// Checks and returns whether a mouse is currently available for input.
        bool IsMouseAvailable() const;

        /// Checks and returns whether a given mouse button is being held down in the current frame.
        bool IsMouseButtonHeld(MouseButton button, bool checkFocus = true) const;

        /// Checks and returns whether a given mouse button is being pressed in the current frame without having been pressed in the
        /// previous frame.
        bool WasMouseButtonPressed(MouseButton button, bool checkFocus = true) const;

        /// Checks and returns whether a given mouse button is not being pressed in the current frame while having been pressed in the
		/// previous frame.
        bool WasMouseButtonReleased(MouseButton button, bool checkFocus = true) const;

        /// Gets the screen position of the mouse, relative to the top-left corner of the screen.
        glm::vec2 GetMousePosition() const;

        /// Gets the delta between the current and previous mouse position, relative to the top-left corner of the screen.
        glm::vec2 GetDeltaMousePosition() const;

        /// Gets the mouse wheel, relative to the value it was the previous frame
        float GetMouseWheel(bool checkFocus = true) const;

        /// Checks and returns whether a keyboard is currently available for input.
        bool IsKeyboardAvailable() const;

        /// Checks and returns whether a given keyboard key is being held down in the current frame.
        bool IsKeyboardKeyHeld(KeyboardKey button, bool checkFocus = true) const;

        /// Checks and returns whether a given keyboard key is being pressed in the current frame without having been pressed in the
        /// previous frame.
        bool WasKeyboardKeyPressed(KeyboardKey button, bool checkFocus = true) const;

        /// Checks and returns whether a given keyboard key is not being pressed in the current frame while having been pressed in the
        /// previous frame.
        bool WasKeyboardKeyReleased(KeyboardKey button, bool checkFocus = true) const;

        /// Returns a range between -1.0f and 1.0f. If the negative is held, it's -1.0f, if the positive is held, it's 1.0f,
        /// if both are held, it returns 0.0f.
        float GetKeyboardAxis(KeyboardKey positive, KeyboardKey negative, bool checkFocus = true) const;

        bool HasFocus() const;

    private:
        bool HasFocus(bool checkFocus) const;

        static constexpr float sTriggerThreshold = 0.15f;

        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(Input);
    };
}

template<>
struct Reflector<CE::Input::KeyboardKey>
{
    static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(KeyBoardKey, CE::Input::KeyboardKey);

template<>
struct Reflector<CE::Input::GamepadAxis>
{
    static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(GamepadAxis, CE::Input::GamepadAxis);

template<>
struct Reflector<CE::Input::GamepadButton>
{
    static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(GamepadButton, CE::Input::GamepadButton);

template<>
struct Reflector<CE::Input::MouseButton>
{
    static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(MouseButton, CE::Input::MouseButton);

template <>
struct magic_enum::customize::enum_range<CE::Input::KeyboardKey> {
    static constexpr int min = 0;
    static constexpr int max = 350;
};
