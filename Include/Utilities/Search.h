#pragma once
#ifdef EDITOR

namespace CE::Search
{
	enum SearchFlags : int32
	{ 
		// Each category and item will be evaluated independently, regardless of the score of
		// it's parent
		IgnoreParentScore = 1,

		// Will not call ImGui::BeginChild before displaying the content.
		// Not to be confused with the general advice given to influencers.
		DontCreateChildForContent = 1 << 1,
	};

	/**
	 * \brief Adds a search bar. Always requires End() to be called.
	 */
	void Begin(SearchFlags flags = {});

	/**
	 * \brief Finishes up by starting to call the submitted display functions, in the order of relevancy to the user.
	 */
	void End();

	/**
	 * \brief A category can contain multiple items and child categories. If the category name closely matches
	 * that of the user's search term, the items in this category will be shown to the user even if those
	 * item's individual names might not closely match the user's search term.
	 *
	 * An example of a commonly used category is CE::Search::TreeNode, it's recommended you use this function if you don't
	 * require a custom implementation.
	 *
	 * \param name The name of the category
	 * \param displayStart A function used to display the start of the category. The std::string_view is the name of the category.
	 * If this returns true, the display functions for items and child categories in this category will also be called. The reason
	 * why we use a display function and not just allow you to make the ImGui calls yourself, is because the order in which the
	 * ImGui calls need to be made changes depending on the user's query, since we sort the items. DisplayStart will only be called
	 * if this category is in some way relevant to the user.
	 */
	void BeginCategory(std::string_view name, std::function<bool(std::string_view)> displayStart);

	/**
	 * \brief Closes the category. 
	 * \param displayEnd If in the displayStart function of BeginCategory you called ImGui::TreeNode, the displayEnd
	 * function could call for example ImGui::TreePop. This function is only called if displayStart returned true.
	 */
	void EndCategory(std::function<void()> displayEnd = {});

	/**
	 * \brief Adds a single named item.
	 *
	 * An example of a commonly used item is CE::Search::Button, it's recommended you use this function if you don't
	 * require a custom implementation.
	 *
	 * \param name The name of the item
	 * \param display A function used to display the item. The std::string_view is the name of the item.
	 * Return true if your button was selected or clicked, in which case this function will also returns true. The reason
	 * why we use a display function and not just allow you to make the ImGui calls yourself, is because the order in which the
	 * ImGui calls need to be made changes depending on the user's query, since we sort the items. Display will only be called
	 * if this category is in some way relevant to the user.
	 * \return Returns true if your custom display function returned true.
	 */
	bool AddItem(std::string_view name, std::function<bool(std::string_view)> display);

	void TreeNode(std::string_view label);

	void TreePop();

	bool Button(std::string_view label);

	bool BeginCombo(std::string_view label, std::string_view previewValue, SearchFlags searchFlags = {});

	void EndCombo();

	bool BeginPopup(std::string_view name, SearchFlags searchFlags = {});

	void EndPopup();

	/**
	 * \brief A value that is added to the score of the most recently added entry when comparing it against
	 * the string the user is looking for.
	 *
	 * Can be used to move commonly used entries higher up.
	 *
	 * Can be used on both items AND categories.
	 *
	 * \param bonus A value that gets added to the base score. The base score for all entries is always
	 * between 0.0f and 1.0f, take this into account when applying your bonus.
	 */
	void SetBonus(float bonus);

	/**
	 * \brief Can be used to find out what the user is currently searching for
	 */
	const std::string& GetUserQuery();
}
#endif // EDITOR