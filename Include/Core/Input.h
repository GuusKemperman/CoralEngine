#pragma once
#include "Meta/MetaReflect.h"
#include "Core/EngineSubsystem.h"

namespace Engine
{
    /// <summary>
	/// The Input class provides a cross-platform way to handle input from keyboard, mouse, and gamepads.
	/// </summary>
    class Input final :
		public EngineSubsystem<Input>
    {
        friend EngineSubsystem<Input>;
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
            /// This button has different names on different platforms, such as Share (Xbox) or - (Switch). The ***REMOVED*** does not have
            /// such a button.
            MenuLeft = 6,
            /// Represents the right of the two menu-related buttons on a gamepad.
            /// This button has different names on different platforms, such as Menu (Xbox), Start (***REMOVED***), or + (Switch).
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
            DPadLeft = 14
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

        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(Input);
    };
}

template<>
struct Reflector<Engine::Input::KeyboardKey>
{
    static Engine::MetaType Reflect();
}; REFLECT_AT_START_UP(KeyBoardKey, Engine::Input::KeyboardKey);


template<>
struct Reflector<Engine::Input::GamepadAxis>
{
    static Engine::MetaType Reflect();
}; REFLECT_AT_START_UP(GamepadAxis, Engine::Input::GamepadAxis);

template<>
struct Reflector<Engine::Input::GamepadButton>
{
    static Engine::MetaType Reflect();
}; REFLECT_AT_START_UP(GamepadButton, Engine::Input::GamepadButton);

template<>
struct Reflector<Engine::Input::MouseButton>
{
    static Engine::MetaType Reflect();
}; REFLECT_AT_START_UP(MouseButton, Engine::Input::MouseButton);


template<>
struct Engine::EnumStringPairsImpl<Engine::Input::KeyboardKey>
{
    static constexpr EnumStringPairs<Input::KeyboardKey, 120> value = {
        EnumStringPair<Input::KeyboardKey>{ Input::KeyboardKey::Space, "Space" },
            { Input::KeyboardKey::Apostrophe, "Apostrophe" },
            { Input::KeyboardKey::Comma, "Comma" },
            { Input::KeyboardKey::Minus, "Minus" },
            { Input::KeyboardKey::Period, "Period" },
            { Input::KeyboardKey::Slash, "Slash" },
            { Input::KeyboardKey::Digit0, "Digit0" },
            { Input::KeyboardKey::Digit1, "Digit1" },
            { Input::KeyboardKey::Digit2, "Digit2" },
            { Input::KeyboardKey::Digit3, "Digit3" },
            { Input::KeyboardKey::Digit4, "Digit4" },
            { Input::KeyboardKey::Digit5, "Digit5" },
            { Input::KeyboardKey::Digit6, "Digit6" },
            { Input::KeyboardKey::Digit7, "Digit7" },
            { Input::KeyboardKey::Digit8, "Digit8" },
            { Input::KeyboardKey::Digit9, "Digit9" },
            { Input::KeyboardKey::Semicolon, "Semicolon" },
            { Input::KeyboardKey::Equal, "Equal" },
            { Input::KeyboardKey::A, "A" },
            { Input::KeyboardKey::B, "B" },
            { Input::KeyboardKey::C, "C" },
            { Input::KeyboardKey::D, "D" },
            { Input::KeyboardKey::E, "E" },
            { Input::KeyboardKey::F, "F" },
            { Input::KeyboardKey::G, "G" },
            { Input::KeyboardKey::H, "H" },
            { Input::KeyboardKey::I, "I" },
            { Input::KeyboardKey::J, "J" },
            { Input::KeyboardKey::K, "K" },
            { Input::KeyboardKey::L, "L" },
            { Input::KeyboardKey::M, "M" },
            { Input::KeyboardKey::N, "N" },
            { Input::KeyboardKey::O, "O" },
            { Input::KeyboardKey::P, "P" },
            { Input::KeyboardKey::Q, "Q" },
            { Input::KeyboardKey::R, "R" },
            { Input::KeyboardKey::S, "S" },
            { Input::KeyboardKey::T, "T" },
            { Input::KeyboardKey::U, "U" },
            { Input::KeyboardKey::V, "V" },
            { Input::KeyboardKey::W, "W" },
            { Input::KeyboardKey::X, "X" },
            { Input::KeyboardKey::Y, "Y" },
            { Input::KeyboardKey::Z, "Z" },
            { Input::KeyboardKey::LeftBracket, "LeftBracket" },
            { Input::KeyboardKey::Backslash, "Backslash" },
            { Input::KeyboardKey::RightBracket, "RightBracket" },
            { Input::KeyboardKey::GraveAccent, "GraveAccent" },
            { Input::KeyboardKey::World1, "World1" },
            { Input::KeyboardKey::World2, "World2" },
            { Input::KeyboardKey::Escape, "Escape" },
            { Input::KeyboardKey::Enter, "Enter" },
            { Input::KeyboardKey::Tab, "Tab" },
            { Input::KeyboardKey::Backspace, "Backspace" },
            { Input::KeyboardKey::Insert, "Insert" },
            { Input::KeyboardKey::Delete, "Delete" },
            { Input::KeyboardKey::ArrowRight, "ArrowRight" },
            { Input::KeyboardKey::ArrowLeft, "ArrowLeft" },
            { Input::KeyboardKey::ArrowDown, "ArrowDown" },
            { Input::KeyboardKey::ArrowUp, "ArrowUp" },
            { Input::KeyboardKey::PageUp, "PageUp" },
            { Input::KeyboardKey::PageDown, "PageDown" },
            { Input::KeyboardKey::Home, "Home" },
            { Input::KeyboardKey::End, "End" },
            { Input::KeyboardKey::CapsLock, "CapsLock" },
            { Input::KeyboardKey::ScrollLock, "ScrollLock" },
            { Input::KeyboardKey::NumLock, "NumLock" },
            { Input::KeyboardKey::PrintScreen, "PrintScreen" },
            { Input::KeyboardKey::Pause, "Pause" },
            { Input::KeyboardKey::F1, "F1" },
            { Input::KeyboardKey::F2, "F2" },
            { Input::KeyboardKey::F3, "F3" },
            { Input::KeyboardKey::F4, "F4" },
            { Input::KeyboardKey::F5, "F5" },
            { Input::KeyboardKey::F6, "F6" },
            { Input::KeyboardKey::F7, "F7" },
            { Input::KeyboardKey::F8, "F8" },
            { Input::KeyboardKey::F9, "F9" },
            { Input::KeyboardKey::F10, "F10" },
            { Input::KeyboardKey::F11, "F11" },
            { Input::KeyboardKey::F12, "F12" },
            { Input::KeyboardKey::F13, "F13" },
            { Input::KeyboardKey::F14, "F14" },
            { Input::KeyboardKey::F15, "F15" },
            { Input::KeyboardKey::F16, "F16" },
            { Input::KeyboardKey::F17, "F17" },
            { Input::KeyboardKey::F18, "F18" },
            { Input::KeyboardKey::F19, "F19" },
            { Input::KeyboardKey::F20, "F20" },
            { Input::KeyboardKey::F21, "F21" },
            { Input::KeyboardKey::F22, "F22" },
            { Input::KeyboardKey::F23, "F23" },
            { Input::KeyboardKey::F24, "F24" },
            { Input::KeyboardKey::F25, "F25" },
            { Input::KeyboardKey::Numpad0, "Numpad0" },
            { Input::KeyboardKey::Numpad1, "Numpad1" },
            { Input::KeyboardKey::Numpad2, "Numpad2" },
            { Input::KeyboardKey::Numpad3, "Numpad3" },
            { Input::KeyboardKey::Numpad4, "Numpad4" },
            { Input::KeyboardKey::Numpad5, "Numpad5" },
            { Input::KeyboardKey::Numpad6, "Numpad6" },
            { Input::KeyboardKey::Numpad7, "Numpad7" },
            { Input::KeyboardKey::Numpad8, "Numpad8" },
            { Input::KeyboardKey::Numpad9, "Numpad9" },
            { Input::KeyboardKey::NumpadDecimal, "NumpadDecimal" },
            { Input::KeyboardKey::NumpadDivide, "NumpadDivide" },
            { Input::KeyboardKey::NumpadMultiply, "NumpadMultiply" },
            { Input::KeyboardKey::NumpadSubtract, "NumpadSubtract" },
            { Input::KeyboardKey::NumpadAdd, "NumpadAdd" },
            { Input::KeyboardKey::NumpadEnter, "NumpadEnter" },
            { Input::KeyboardKey::NumpadEqual, "NumpadEqual" },
            { Input::KeyboardKey::LeftShift, "LeftShift" },
            { Input::KeyboardKey::LeftControl, "LeftControl" },
            { Input::KeyboardKey::LeftAlt, "LeftAlt" },
            { Input::KeyboardKey::LeftSuper, "LeftSuper" },
            { Input::KeyboardKey::RightShift, "RightShift" },
            { Input::KeyboardKey::RightControl, "RightControl" },
            { Input::KeyboardKey::RightAlt, "RightAlt" },
            { Input::KeyboardKey::RightSuper, "RightSuper" },
            { Input::KeyboardKey::Menu, "Menu" },
    };
};

template<>
struct Engine::EnumStringPairsImpl<Engine::Input::GamepadAxis>
{
    static constexpr EnumStringPairs<Input::GamepadAxis, 6> value = {
        EnumStringPair<Input::GamepadAxis>{ Input::GamepadAxis::StickLeftX, "StickLeftX" },
        { Input::GamepadAxis::StickLeftY, "StickLeftY" },
        { Input::GamepadAxis::StickRightX, "StickRightX" },
        { Input::GamepadAxis::StickRightY, "StickRightY" },
        { Input::GamepadAxis::TriggerLeft, "TriggerLeft" },
        { Input::GamepadAxis::TriggerRight, "TriggerRight" },
    };
};

template<>
struct Engine::EnumStringPairsImpl<Engine::Input::GamepadButton>
{
    static constexpr EnumStringPairs<Input::GamepadButton, 14> value = {
        EnumStringPair<Input::GamepadButton>{ Input::GamepadButton::South, "South" },
        { Input::GamepadButton::East, "East" },
        { Input::GamepadButton::West, "West" },
        { Input::GamepadButton::North, "North" },
        { Input::GamepadButton::ShoulderLeft, "ShoulderLeft" },
        { Input::GamepadButton::ShoulderRight, "ShoulderRight" },
        { Input::GamepadButton::MenuLeft, "MenuLeft" },
        { Input::GamepadButton::MenuRight, "MenuRight" },
        { Input::GamepadButton::StickPressLeft, "StickPressLeft" },
        { Input::GamepadButton::StickPressRight, "StickPressRight" },
        { Input::GamepadButton::DPadUp, "DPadUp" },
        { Input::GamepadButton::DPadRight, "DPadRight" },
        { Input::GamepadButton::DPadDown, "DPadDown" },
        { Input::GamepadButton::DPadLeft, "DPadLeft" }
    };
};

template<>
struct Engine::EnumStringPairsImpl<Engine::Input::MouseButton>
{
    static constexpr EnumStringPairs<Input::MouseButton, 14> value = {
        EnumStringPair<Input::MouseButton>{ Input::MouseButton::Left, "Left" },
        { Input::MouseButton::Right, "Right" },
        { Input::MouseButton::Middle, "Middle" },
    };
};

