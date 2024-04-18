#pragma once
#ifdef EDITOR

namespace CE::Search
{
	enum SearchFlags : int32
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

	void TreeNode(std::string_view label);

	void TreePop();

	bool Button(std::string_view label);

	bool BeginCombo(std::string_view label, std::string_view previewValue, SearchFlags searchFlags = {});

	void EndCombo();

	bool BeginPopup(std::string_view name, SearchFlags searchFlags = {});

	void EndPopup();

	/**
	 * \brief A value that is added to the score of the most recently added entry when comparing it against the string the user is looking for.
	 *
	 * Can be used to move commonly used entries higher up.
	 *
	 * \param bonus A value that gets added to the base score. The base score for all entries is always between 0.0f and 1.0f, take this into account when applying your bonus.
	 */
	void SetBonus(float bonus);
}
#endif // EDITOR