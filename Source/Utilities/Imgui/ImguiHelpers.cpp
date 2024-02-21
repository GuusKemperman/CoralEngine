#include "Precomp.h"
#include "Utilities/Imgui/ImguiHelpers.h"

#include "imgui/imgui_internal.h"

bool ImGui::CollapsingHeaderWithButton(const char* headerText, const char* buttonText, bool* buttonPressed, ImGuiTreeNodeFlags headerFlags)
{
	SetNextItemAllowOverlap();
	const bool isCollapsingHeaderOpen = CollapsingHeader(headerText, headerFlags);
	SameLine();

	const float buttonWidth = CalcTextSize(buttonText, NULL, true).x;

	SameLine(GetWindowWidth() - buttonWidth - 16.0f);

	PushID(headerText);

	const bool isButtonPressed = SmallButton(buttonText);

	PopID();

	if (buttonPressed != nullptr)
	{
		*buttonPressed = isButtonPressed;
	}

	return isCollapsingHeaderOpen;
}

bool ImGui::Splitter(bool split_vertically, float* size1, float* size2, float min_size1, float min_size2, float thickness, float splitter_long_axis_size)
{
	using namespace ImGui;
	const ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	const ImGuiID id = window->GetID("##Splitter");
	ImRect bb;
	bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
	bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
	bool changed = SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);

	const float totalSize = *size1 + *size2;
	const float availSize = split_vertically ? GetContentRegionAvail().x : GetContentRegionAvail().y;
	if (fabsf(totalSize - availSize) > 0.01f)
	{
		// Calculate ratio
		const float size1PercentageOfTotal = *size1 / totalSize;
		*size1 = availSize * size1PercentageOfTotal;
		*size2 = availSize * (1.0f - size1PercentageOfTotal);
		changed = true;
	}

	return changed;
}
