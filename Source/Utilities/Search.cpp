#include "Precomp.h"
#include "Utilities/Search.h"

#include <future>
#include <stack>

#include "Utilities/ManyStrings.h"
#include "rapidfuzz/rapidfuzz_all.hpp"

using CachedScorer = rapidfuzz::fuzz::CachedPartialTokenSortRatio<char>;

namespace
{
	struct CategoryFunctions
	{
		std::function<bool(std::string_view)> mOnDisplayStart{};
		std::function<void()> mOnDisplayEnd{};
	};

	struct ItemFunctions
	{
		std::function<bool(std::string_view)> mOnDisplay{};
	};

	// Can represent either
	// a category or an item
	struct Entry
	{
		uint32 mIsCategory : 1;
		uint32 mNumOfTotalChildren : 31;
	};

	struct SearchContext
	{
		CE::ManyStrings mNames{};
		std::vector<Entry> mAllEntries{};
		std::vector<std::variant<CategoryFunctions, ItemFunctions>> mDisplayFunctions{};

		std::stack<uint32> mCategoryStack{};

		std::string mUserQuery{};

		struct CalculationResult
		{
			// Inputs
			CE::ManyStrings mNames{};
			std::vector<Entry> mEntries{};
			std::string mUserQuery{};

			// Outputs
			std::vector<bool> mIsItemVisible{};
			std::vector<uint32> mIndicesOfTopEntries{};
		};
		std::optional<CalculationResult> mCalculationResult{};

		uint32 mIndexOfPressedItem = std::numeric_limits<uint32>::max();
		CE::Search::SearchFlags mFlags{};
	};
	std::unordered_map<ImGuiID, SearchContext> sContexts{};
	std::stack<std::reference_wrapper<SearchContext>> sContextStack{};

	constexpr std::string_view sDefaultLabel = ICON_FA_SEARCH "##SearchBar";
	constexpr std::string_view sDefaultHint = "Search";


	void ProcessItemClickConsumption(SearchContext& context);
	void RecursivelyDisplayEntry(SearchContext& context, uint32& index, uint32 stopAt);
	void ApplyKeyboardNavigation(SearchContext& context);
}

void CE::Search::Begin(std::string_view id, SearchFlags flags)
{
	const ImGuiID imId = ImGui::GetID(id.data());
	ImGui::PushID(imId);

	SearchContext& context = sContextStack.emplace(sContexts[imId]);
	context.mFlags = flags;

	ImGui::InputTextWithHint(sDefaultLabel.data(), sDefaultHint.data(), &context.mUserQuery);
}

void CE::Search::End()
{
	// Display all the items here
	SearchContext& context = sContextStack.top();

	if ((context.mFlags & SearchFlags::NoKeyboardSelect) == 0)
	{
		ApplyKeyboardNavigation(context);
	}

	context.mCalculationResult.emplace(SearchContext::CalculationResult
		{
			context.mNames,
			context.mAllEntries,
			context.mUserQuery,
			std::vector<bool>(context.mAllEntries.size(), true)
		});

	uint32 index{};
	RecursivelyDisplayEntry(context, index, static_cast<uint32>(context.mAllEntries.size()));

	context.mAllEntries.clear();
	context.mDisplayFunctions.clear();
	context.mNames.Clear();
	ASSERT_LOG(context.mCategoryStack.empty(), "There were more calls to BeginCategory than to EndCategory");

	sContextStack.pop();
	ImGui::PopID();
}

void CE::Search::BeginCategory(std::string_view name, std::function<bool(std::string_view)> displayStart)
{
	SearchContext& context = sContextStack.top();
	context.mAllEntries.emplace_back(
		Entry
		{
			true,
			0
		});
	context.mDisplayFunctions.emplace_back(CategoryFunctions{ std::move(displayStart) });
	context.mNames.Emplace(name);
	context.mCategoryStack.emplace(static_cast<uint32>(context.mAllEntries.size()) - 1);
}

void CE::Search::EndCategory(std::function<void()> displayEnd)
{
	SearchContext& context = sContextStack.top();

	// We need to update display end
	const uint32 indexOfCurrentCategory = context.mCategoryStack.top();

	std::variant<CategoryFunctions, ItemFunctions>& funcs = context.mDisplayFunctions[indexOfCurrentCategory];
	CategoryFunctions& catFunctions = std::get<CategoryFunctions>(funcs);
	catFunctions.mOnDisplayEnd = std::move(displayEnd);

	context.mAllEntries[indexOfCurrentCategory].mNumOfTotalChildren = static_cast<uint32>(context.mAllEntries.size() - 1) - indexOfCurrentCategory;


	context.mCategoryStack.pop();
}

bool CE::Search::AddEntry(std::string_view name, std::function<bool(std::string_view)> display)
{
	SearchContext& context = sContextStack.top();

	const bool wasPressed = context.mIndexOfPressedItem == context.mAllEntries.size();

	if (wasPressed)
	{
		ProcessItemClickConsumption(context);
	}

	context.mAllEntries.emplace_back(
		Entry
		{
			false,
			0
		});
	context.mDisplayFunctions.emplace_back(ItemFunctions{ std::move(display) });
	context.mNames.Emplace(name);

	return wasPressed;
}

bool CE::Search::BeginCombo(std::string_view label, std::string_view previewValue, ImGuiComboFlags flags)
{
	if (!ImGui::BeginCombo(label.data(), previewValue.data(), flags))
	{
		return false;
	}

	Begin(label);

	return true;
}

bool CE::Search::Button(std::string_view label)
{
	return AddEntry(label,
		[](std::string_view name)
		{
			// If we click on the button, the search term is reset, the button may move,
			// and the entire popup is closed.
			// We won't be hovering over the button when the mouse button is released.
			// So we activate OnClick and not OnRelease
			return ImGui::MenuItem(name.data()) || ImGui::IsItemClicked(0);
		}
	);
}

void CE::Search::EndCombo()
{
	End();
	ImGui::EndCombo();
}

void CE::Search::TreeNode(std::string_view label)
{
	BeginCategory(label, [](std::string_view l) { return ImGui::TreeNode(l.data()); });
}

void CE::Search::TreePop()
{
	EndCategory([] { ImGui::TreePop(); });
}

bool CE::Search::BeginPopup(std::string_view name)
{
	ImGui::SetNextWindowSize(ImVec2{ -1.0f, 300.0f });

	if (!ImGui::BeginPopup(name.data()))
	{
		return false;
	}

	Begin(std::string{ name } + "SearchInPopUp");
	return true;
}

void CE::Search::EndPopup()
{
	End();
	ImGui::EndPopup();
}

namespace
{
	void ProcessItemClickConsumption(SearchContext& context)
	{
		context.mIndexOfPressedItem = std::numeric_limits<uint32>::max();
	}

	void RecursivelyDisplayEntry(SearchContext& context, uint32& index, uint32 stopAt)
	{
		const SearchContext::CalculationResult& result = *context.mCalculationResult;

		for (; index < stopAt; index++)
		{
			if (!result.mIsItemVisible[index])
			{
				continue;
			}

			const std::string_view name = context.mNames[index];
			const Entry& entry = context.mAllEntries[index];
			const std::variant<CategoryFunctions, ItemFunctions>& functions = context.mDisplayFunctions[index];

			//if (isSelected)
			//{
			//	ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
			//}

			if (entry.mIsCategory)
			{
				const CategoryFunctions& catFunctions = std::get<CategoryFunctions>(functions);

				if (!catFunctions.mOnDisplayStart 
					|| catFunctions.mOnDisplayStart(name))
				{
					++index;

					RecursivelyDisplayEntry(context, index, index + entry.mNumOfTotalChildren);


					// We incremented the index already, we undo
					// that here to prevent the increment in our for-loop
					// from doing this operation twice.
					index--;

					if (catFunctions.mOnDisplayEnd)
					{
						catFunctions.mOnDisplayEnd();
					}
				}
				else
				{
					index += entry.mNumOfTotalChildren;
				}
			}
			else
			{
				const ItemFunctions& itemFunctions = std::get<ItemFunctions>(functions);

				if (itemFunctions.mOnDisplay 
					&& itemFunctions.mOnDisplay(name)
					/*|| (isSelected && ImGui::IsKeyPressed(ImGuiKey_Enter))*/)
				{
					context.mIndexOfPressedItem = index;
				}
			}

			//if (isSelected)
			//{
			//	ImGui::PopStyleColor();
			//}
		}
	}

	void ApplyKeyboardNavigation(SearchContext& /*context*/)
	{
		//// Update focus based on the arrow keys
		//if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
		//{
		//	context.mIndexOfSelectedItem++;
		//	if (isTyping)
		//	{
		//		context.mIndexOfSelectedItem = 0;
		//	}
		//	else if (context.mIndexOfSelectedItem + 1 < context.mAllEntries.size())
		//	{
		//		context.mIndexOfSelectedItem++;

		//	}
		//}
		//else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
		//{
		//	if (context.mIndexOfSelectedItem == 0)
		//	{
		//		// Return focus to the search bar
		//		ImGui::SetKeyboardFocusHere(-1);
		//	}
		//	else
		//	{
		//		context.mIndexOfSelectedItem--;
		//	}
		//}
	}
}

//std::optional<std::string> CE::Search::DisplaySearchBar(const ManyStrings& strings, std::string_view label, std::string_view hint)
//{
//    if (!BeginSearchCombo(label, hint))
//    {
//        return std::nullopt;
//    }
//
//    struct SearchResult
//    {
//        ~SearchResult()
//        {
//            if (mPendingScores.joinable())
//            {
//                mPendingScores.join();
//            }
//        }
//
//        std::thread mPendingScores{};
//        bool mIsReady{};
//        std::string mQuery{};
//        ManyStrings mStrings{};
//        std::vector<float> mScores{};
//        std::vector<uint32> mIndicesOfSortedStrings{};
//    };
//
//    static std::array<SearchResult, 2> sResults{};
//    static bool sLastValidResult{};
//
//    // Check if the result from our previous thread is ready
//    if (sResults[!sLastValidResult].mIsReady)
//    {
//        // Note that we do not check if the pending result is still valid.
//        // We do this later, where we check if the last valid result is
//        // still valid. Since our pending result now becomes our last valid result,
//        // we can defer that check.
//        SearchResult& pendingResult = sResults[!sLastValidResult];
//
//        if (pendingResult.mPendingScores.joinable())
//        {
//            pendingResult.mPendingScores.join();
//        }
//        pendingResult.mIsReady = false;
//
//        // Swap the buffers
//        sLastValidResult = !sLastValidResult;
//    }
//
//    SearchResult& lastValidResult = sResults[sLastValidResult];
//    bool isCurrentResultInvalid{};
//
//    // In case the list of strings to search through has changed,
//    // we need to update our last valid result.
//    if (lastValidResult.mStrings != strings)
//    {
//        // Our last valid result has been invalidated, just assume all the terms match
//        lastValidResult.mScores.resize(strings.NumOfStrings());
//        std::fill(lastValidResult.mScores.begin(), lastValidResult.mScores.end(), 100.0f);
//
//        // Make a copy here, but we set
//        // shouldRelaunch to true, so the
//        // strings will be moved into our
//        // pending query.
//        lastValidResult.mStrings = strings;
//        isCurrentResultInvalid = true;
//    }
//
//    // Show the current options
//    static std::string sCurrentQuery{};
//    DisplaySearchBar(sCurrentQuery, label, hint);
//
//    std::optional<std::string> returnValue{};
//
//    for (const uint32 index : lastValidResult.mIndicesOfSortedStrings)
//    {
//        const std::string_view item = lastValidResult.mStrings[index];
//        if (ImGui::Selectable(item.data())
//            || ImGui::IsItemClicked()) // If we click on the button, the search term is reset, the button may move, 
//            // and we won't be hovering over the button when the mouse button is released.
//            // So we activate OnClick and not OnRelease)
//        {
//            returnValue.emplace(item);
//        }
//    }
//
//    // If the search term has changed AND we don't already have
//    // a query pending, we make a new query.
//    if (isCurrentResultInvalid
//        || (sCurrentQuery != lastValidResult.mQuery
//            && !sResults[!sLastValidResult].mPendingScores.joinable()))
//    {
//        SearchResult& pending = sResults[!sLastValidResult];
//        pending.mStrings = strings;
//        pending.mQuery = sCurrentQuery;
//        pending.mScores.clear();
//        pending.mIsReady = false;
//        pending.mPendingScores = std::thread{
//            [&pending]
//            {
//                pending.mScores.resize(pending.mStrings.NumOfStrings());
//
//                CachedScorer scorer{ pending.mQuery };
//
//                for (size_t i = 0; i < pending.mStrings.NumOfStrings(); i++)
//                {
//                    pending.mScores[i] = static_cast<float>(scorer.similarity(pending.mStrings[i], sDefaultCutOff));
//                }
//
//                pending.mIndicesOfSortedStrings.resize(pending.mScores.size());
//                std::iota(pending.mIndicesOfSortedStrings.begin(), pending.mIndicesOfSortedStrings.end(), 0);
//
//                std::sort(pending.mIndicesOfSortedStrings.begin(), pending.mIndicesOfSortedStrings.end(),
//                    [&pending](uint32 lhs, uint32 rhs)
//                    {
//                        return pending.mScores[lhs] > pending.mScores[rhs];
//                    });
//
//                pending.mIsReady = true;
//            }
//        };
//    }
//
//    EndSearchCombo(returnValue.has_value());
//
//    return returnValue;
//}