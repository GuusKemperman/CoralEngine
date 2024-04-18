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

	enum WindowFlags : int32
	{
		/**
		 * \brief Normally if BeginCombo or BeginPopup returns true,
		 * CE::Search::Begin will have already been called for you.
		 * With this flag, you can prevent that from happening, which allows
		 * you to do something like this:
		 *
		 * ImGui::Button("Ha!"); ImGui::SameLine(); CE::Search::Begin("Searching");
		 *
		 * This places a button on the same line as the actual search bar.
		 */
		CallSearchBeginManually = 1 << 1
	};

	bool BeginCombo(std::string_view label, std::string_view previewValue, int32 searchFlagsAndWindowFlags = {});

	void EndCombo();

	bool BeginPopup(std::string_view name, int32 searchFlagsAndWindowFlags = {});

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