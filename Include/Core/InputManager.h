#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class InputManager
	{
	public:
		static void NewFrame();

		static glm::vec2 GetMousePos();

		// Will return true the first frame this key is pressed.
		static bool IsKeyPressed(ImGuiKey key, bool checkFocus = true);

		// Will return true the entire duration that the key is held down.
		static bool IsKeyDown(ImGuiKey key, bool checkFocus = true);

		// Will return true the first frame the key is released.
		static bool IsKeyReleased(ImGuiKey key, bool checkFocus = true);

		// Will return a value withing the range of -1.0 to 1.0
		// Works with gamepads, for example GetAxis(ImGuiKey_GamepadLStickUp, ImGuiKey_GamepadLStickDown);
		static float GetAxis(ImGuiKey positive, ImGuiKey negative, bool checkFocus = true);

		static float GetScrollY(bool checkFocus = true);

		static bool HasFocus();


	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(InputManager);
	};
}
