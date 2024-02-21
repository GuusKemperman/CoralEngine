# define IMGUI_DEFINE_MATH_OPERATORS
# include "widgets.h"
# include <imgui/imgui_internal.h>

void ax::Widgets::Icon(const ImVec2& size, IconType type, bool filled, bool mirrored, const ImVec4& color/* = ImVec4(1, 1, 1, 1)*/, const ImVec4& innerColor/* = ImVec4(0, 0, 0, 0)*/)
{
	if (ImGui::IsRectVisible(size))
	{
		auto cursorPos = ImGui::GetCursorScreenPos();
		auto drawList = ImGui::GetWindowDrawList();

		if (mirrored)
		{
			ax::Drawing::DrawIcon(drawList, cursorPos + size, cursorPos, type, filled, ImColor(color), ImColor(innerColor));
		}
		else
		{
			ax::Drawing::DrawIcon(drawList, cursorPos, cursorPos + size, type, filled, ImColor(color), ImColor(innerColor));
		}
	}

	ImGui::Dummy(size);
}

