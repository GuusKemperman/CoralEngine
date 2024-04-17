#pragma once
#ifdef EDITOR

namespace CE::Search
{
	enum SearchFlags
	{
		// Each category and item will be evaluated independently, regardless of the score of
		// it's parent
		IgnoreParentScore = 1,
	};

	void Begin(std::string_view id, SearchFlags flags = {});

	void End();

	void BeginCategory(std::string_view name, std::function<bool(std::string_view)> displayStart);

	void EndCategory(std::function<void()> displayEnd);

	bool AddItem(std::string_view name, std::function<bool(std::string_view)> display);

	bool BeginCombo(std::string_view label, std::string_view previewValue, ImGuiComboFlags flags = 0);

	bool Button(std::string_view label);

	void EndCombo();

	void TreeNode(std::string_view label);

	void TreePop();

	bool BeginPopup(std::string_view name);

	void EndPopup();
}
#endif // EDITOR