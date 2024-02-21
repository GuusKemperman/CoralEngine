#pragma once

namespace ImGui
{
	bool CollapsingHeaderWithButton(const char* headerText, const char* buttonText, bool* buttonPressed, ImGuiTreeNodeFlags headerFlags = 0);

	/*
	Initial values of size1 and size2 can be the ratio you desire, e.g., .75f and .25f.
	Don't forget to use PushId and PopId when using multiple splitters in the same window.

	Example: 
		Splitter(true, &mViewportWidth, &mHierarchyAndDetailsWidth);

		// Display the viewport
		if (ImGui::BeginChild("WorldViewport", { mViewportWidth, 0.0f }))
		{
			// Your window contents here

		}
		ImGui::EndChild();	
		ImGui::SameLine();

		if (ImGui::BeginChild("HierarchyAndDetailsWindow", { mHierarchyAndDetailsWidth, 0.0f }))
		{
			// Your window contents here
		}
		ImGui::EndChild();
	*/
	bool Splitter(bool split_vertically, float* size1, float* size2,
		float min_size1 = 50.0f, float min_size2 = 50.0f, float thickness = 4.0f, float splitter_long_axis_size = -1.0f);
}