#include "Precomp.h"
#include "Utilities/Search.h"

#include <future>
#include <stack>

#include "Utilities/ManyStrings.h"
#include "rapidfuzz/rapidfuzz_all.hpp"

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
			struct Inputs
			{
				CE::ManyStrings mNames{};
				std::vector<Entry> mEntries{};
				std::string mUserQuery{};
			};
			Inputs mInput{};

			struct InternalReusableBuffers
			{
				std::vector<float> mScores{};
				std::vector<bool> mIsVisible{};
			};
			InternalReusableBuffers mReusableBuffers{};

			struct Outputs
			{
				static constexpr uint32 sDisplayEndOfCategoryFlag = 1u << 31u;
				std::vector<uint32> mDisplayOrder{};
			};
			Outputs mOutput{};

		};
		std::optional<CalculationResult> mCalculationResult{};

		uint32 mIndexOfPressedItem = std::numeric_limits<uint32>::max();
		CE::Search::SearchFlags mFlags{};
	};
	std::unordered_map<ImGuiID, SearchContext> sContexts{};
	std::stack<std::reference_wrapper<SearchContext>> sContextStack{};

	constexpr std::string_view sDefaultLabel = ICON_FA_SEARCH "##SearchBar";
	constexpr std::string_view sDefaultHint = "Search";

	void QuicklyCreateValidResult(SearchContext::CalculationResult& result);
	void UpdateCalculation(SearchContext::CalculationResult& output);
	void ProcessItemClickConsumption(SearchContext& context);
	void DisplayToUser(SearchContext& context);
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

	context.mCalculationResult.emplace(SearchContext::CalculationResult
		{
			context.mNames,
			context.mAllEntries,
			context.mUserQuery,
		});

	QuicklyCreateValidResult(*context.mCalculationResult);
	// UpdateCalculation(*context.mCalculationResult);
	DisplayToUser(context);

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

bool CE::Search::AddItem(std::string_view name, std::function<bool(std::string_view)> display)
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
	return AddItem(label,
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
	struct EntryAsNode
	{
		EntryAsNode(uint32& index, const SearchContext::CalculationResult& result) :
			mIndex(index++)
		{
			const Entry& entry = result.mInput.mEntries[mIndex];

			if (!entry.mIsCategory)
			{
				return;
			}

			while (index <= mIndex + entry.mNumOfTotalChildren)
			{
				mChildren.emplace_back(index, result);
			}
		}

		void AppendToDisplayOrder(SearchContext::CalculationResult& result) const
		{
			result.mOutput.mDisplayOrder.emplace_back(mIndex);

			for (const EntryAsNode& child : mChildren)
			{
				child.AppendToDisplayOrder(result);
			}

			if (result.mInput.mEntries[mIndex].mIsCategory)
			{
				result.mOutput.mDisplayOrder.emplace_back(mIndex | SearchContext::CalculationResult::Outputs::sDisplayEndOfCategoryFlag);
			}
		}

		uint32 mIndex{};
		std::vector<EntryAsNode> mChildren{};
	};

	void QuicklyCreateValidResult(SearchContext::CalculationResult& result)
	{
		std::vector<EntryAsNode> mNodes{};
		
		for (uint32 i = 0; i < result.mInput.mEntries.size();)
		{
			mNodes.emplace_back(i, result);
		}

		for (const EntryAsNode& node : mNodes)
		{
			node.AppendToDisplayOrder(result);
		}
	}

	void UpdateCalculation(SearchContext::CalculationResult& /*output*/)
	{
	/*	if (output.mUserQuery.empty())
		{
			for (size_t i = 0; i < output.mIsVisible.size(); i++)
			{
				output.mIsVisible[i] = true;
			}
			output.mIsVisible.resize(output.mEntries.size(), true);
			return;
		}

		output.mScores.resize(output.mEntries.size());

		const rapidfuzz::fuzz::CachedPartialTokenSortRatio scorer{ output.mUserQuery };

		for (size_t i = 0; i < output.mNames.NumOfStrings(); i++)
		{
			output.mScores[i] = static_cast<float>(scorer.similarity(output.mNames[i]));
		}

		static constexpr float cutOff = 40.0f;
		output.mIsVisible.resize(output.mScores.size());

		for (size_t i = 0; i < output.mEntries.size(); i++)
		{
			output.mIsVisible[i] = output.mScores[i] >= cutOff;
		}*/
	}

	void ProcessItemClickConsumption(SearchContext& context)
	{
		context.mIndexOfPressedItem = std::numeric_limits<uint32>::max();
	}

	void DisplayToUser(SearchContext& context)
	{
		const std::vector<uint32>& displayOrder = context.mCalculationResult->mOutput.mDisplayOrder;
		static constexpr uint32 endOfCatFlag = SearchContext::CalculationResult::Outputs::sDisplayEndOfCategoryFlag;

		for (auto displayCommand = displayOrder.begin(); displayCommand != displayOrder.end(); ++displayCommand)
		{
			const uint32 index = *displayCommand & (~endOfCatFlag);

			const std::string_view name = context.mNames[index];
			const Entry& entry = context.mAllEntries[index];
			const std::variant<CategoryFunctions, ItemFunctions>& functions = context.mDisplayFunctions[index];

			if (!entry.mIsCategory)
			{
				const ItemFunctions& itemFunctions = std::get<ItemFunctions>(functions);

				if (itemFunctions.mOnDisplay
					&& itemFunctions.mOnDisplay(name))
				{
					context.mIndexOfPressedItem = index;
				}

				continue;
			}

			const CategoryFunctions& catFunctions = std::get<CategoryFunctions>(functions);

			if (*displayCommand & endOfCatFlag)
			{
				// Close the tab
				if (catFunctions.mOnDisplayEnd)
				{
					catFunctions.mOnDisplayEnd();
				}

				continue;
			}

			// Open the tab
			if (!catFunctions.mOnDisplayStart
				|| catFunctions.mOnDisplayStart(name)) 
			{
				// If it's open, do nothing
				continue;
			}

			// Otherwise, skip all the items until we encounter an exit
			uint32 numOfCategoriesEncountered = 1;
			++displayCommand;

			for (; displayCommand != displayOrder.end(); ++displayCommand)
			{
				const uint32 indexOfEntryToSkip = *displayCommand & (~endOfCatFlag);
				const Entry& entryToSkip = context.mAllEntries[indexOfEntryToSkip];

				if (!entryToSkip.mIsCategory)
				{
					continue;
				}

				if (*displayCommand & endOfCatFlag)
				{
					if (--numOfCategoriesEncountered == 0)
					{
						break;
					}
				}
				else
				{
					numOfCategoriesEncountered++;
				}
			}
		}
	}
}
