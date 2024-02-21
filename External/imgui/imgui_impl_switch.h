#pragma once

#include "imgui.h"

IMGUI_API bool ImGui_Impl_Switch_Init();
IMGUI_API void ImGui_Impl_Switch_Shutdown();
IMGUI_API void ImGui_Impl_Switch_NewFrame();
IMGUI_API void ImGui_Impl_Switch_RenderDrawData(ImDrawData* data);
