#include "Precomp.h"
#include "Utilities/Search.h"

#include "rapidfuzz/rapidfuzz_all.hpp"
#include <stack>
#include <imgui/imgui_internal.h>

#include "Utilities/ASync.h"
#include "Utilities/ManyStrings.h"
#include "Utilities/Math.h"

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

	struct Result;

	struct EntryAsNode
	{
		EntryAsNode(uint32& index, const Result& result);

		uint32 mIndex{};
		std::vector<EntryAsNode> mChildren{};
	};

	struct Input
	{
		CE::ManyStrings mNames{};
		std::vector<Entry> mEntries{};
		std::vector<float> mBonuses{};

		std::string mUserQuery{};
		CE::Search::SearchFlags mFlags{};
	};

	struct ReusableBuffers
	{
		std::vector<float> mScores{};
		std::vector<EntryAsNode> mNodes{};
		CE::ManyStrings mPreprocessedNames{};
	};

	struct Output
	{
		static constexpr uint32 sDisplayEndOfCategoryFlag = 1u << 31u;
		std::vector<uint32> mDisplayOrder{};
	};

	struct Result
	{
		~Result();

		CE::ASyncThread mThread{};
        bool mIsReady{};

		Input mInput{};
		ReusableBuffers mBuffers{};
		Output mOutput{};
	};

	struct SearchContext
	{
		Input mInput{};

		std::vector<std::variant<CategoryFunctions, ItemFunctions>> mDisplayFunctions{};
		std::stack<uint32> mCategoryStack{};

		std::array<Result, 2> mResults{};
		bool mIndexOfLastValidResult{};

		uint32 mIndexOfPressedItem = std::numeric_limits<uint32>::max();
		bool mHasPressedItemBeenConsumed{};

		// When the user presses enter while typing,
		// we say that the first item was pressed.
		bool mHasEnterBeenConsumed{};
	};
	std::unordered_map<ImGuiID, SearchContext> sContexts{};
	std::stack<std::reference_wrapper<SearchContext>> sContextStack{};

	constexpr std::string_view sDefaultLabel = ICON_FA_SEARCH "##SearchBar";
	constexpr std::string_view sDefaultHint = "Search";

	constexpr bool sDebugPrintingEnabled = false;

	// Increasing this value will reduce the amount of
	// items shown to the user.
	constexpr float sFilterStrength = .5f;

	bool operator==(const Entry& lhs, const Entry& rhs);
	bool operator!=(const Entry& lhs, const Entry& rhs);

	void BeginChild();
	bool IsResultSafeToUse(const Result& oldResult, const Input& currentInput);
	bool IsResultUpToDate(const Result& oldResult, const Input& currentInput);
	void BringResultUpToDate(Result& result);
	void ProcessItemClickConsumption(SearchContext& context);
	void DisplayToUser(SearchContext& context);
}

void CE::Search::Begin(SearchFlags flags)
{
	const ImGuiID imId = ImGui::GetID("Search");
	ImGui::PushID(imId);

	SearchContext& context = sContextStack.emplace(sContexts[imId]);
	context.mInput.mFlags = flags;

	ImGui::InputTextWithHint(sDefaultLabel.data(), sDefaultHint.data(), &context.mInput.mUserQuery);

	context.mHasEnterBeenConsumed = !ImGui::IsItemFocused()
		|| !ImGui::IsKeyPressed(ImGuiKey_Enter);
}

void CE::Search::End()
{
	// Display all the items here
	SearchContext& context = sContextStack.top();

	// Check if the result from our previous thread is ready
    if (context.mResults[!context.mIndexOfLastValidResult].mIsReady)
    {
        // Note that we do not check if the pending result is still valid.
        // We do this later, where we check if the last valid result is
        // still valid. Since our pending result now becomes our last valid result,
        // we can defer that check.
        Result& pendingResult = context.mResults[!context.mIndexOfLastValidResult];

        if (pendingResult.mThread.WasLaunched())
        {
            pendingResult.mThread.Join();
        }
        pendingResult.mIsReady = false;

        // Swap the buffers
		context.mIndexOfLastValidResult = !context.mIndexOfLastValidResult;
    }

    Result& lastValidResult = context.mResults[context.mIndexOfLastValidResult];

    // In case the list of strings to search through has changed,
    // we need to update our last valid result.
    if (!IsResultSafeToUse(lastValidResult, context.mInput))
    {
		lastValidResult.mInput = context.mInput;
		BringResultUpToDate(lastValidResult);
    }

	DisplayToUser(context);

	Result& pendingResult = context.mResults[!context.mIndexOfLastValidResult];

	if (!IsResultUpToDate(lastValidResult, context.mInput)
		&& !pendingResult.mThread.WasLaunched())
	{
		pendingResult.mInput = context.mInput;
		pendingResult.mIsReady = false;

		pendingResult.mThread =
		{
			[&pendingResult]
			{
				BringResultUpToDate(pendingResult);
				pendingResult.mIsReady = true;
			}
		};
	}

	if (!context.mInput.mEntries.empty()
		&& (context.mInput.mFlags & SearchFlags::DontCreateChildForContent) == 0)
	{
		ImGui::EndChild();
	}

	ImGui::PopID();

	context.mInput.mEntries.clear();
	context.mInput.mNames.Clear();
	context.mInput.mBonuses.clear();
	context.mDisplayFunctions.clear();
	ASSERT_LOG(context.mCategoryStack.empty(), "There were more calls to BeginCategory than to EndCategory");

	sContextStack.pop();
}

void CE::Search::BeginCategory(std::string_view name, std::function<bool(std::string_view)> displayStart)
{
	SearchContext& context = sContextStack.top();

	if (context.mInput.mEntries.empty()
		&& (context.mInput.mFlags & SearchFlags::DontCreateChildForContent) == 0)
	{
		BeginChild();
	}

	context.mInput.mEntries.emplace_back(
		Entry
		{
			true,
			0
		});
	context.mInput.mNames.Emplace(name);
	context.mInput.mBonuses.emplace_back();
	context.mDisplayFunctions.emplace_back(CategoryFunctions{ std::move(displayStart) });
	context.mCategoryStack.emplace(static_cast<uint32>(context.mInput.mEntries.size()) - 1);
}

void CE::Search::EndCategory(std::function<void()> displayEnd)
{
	SearchContext& context = sContextStack.top();

	// We need to update display end
	const uint32 indexOfCurrentCategory = context.mCategoryStack.top();

	std::variant<CategoryFunctions, ItemFunctions>& funcs = context.mDisplayFunctions[indexOfCurrentCategory];
	CategoryFunctions& catFunctions = std::get<CategoryFunctions>(funcs);
	catFunctions.mOnDisplayEnd = std::move(displayEnd);

	context.mInput.mEntries[indexOfCurrentCategory].mNumOfTotalChildren = static_cast<uint32>(context.mInput.mEntries.size() - 1) - indexOfCurrentCategory;
	context.mCategoryStack.pop();
}

bool CE::Search::AddItem(std::string_view name, std::function<bool(std::string_view)> display)
{
	SearchContext& context = sContextStack.top();

	if (context.mInput.mEntries.empty()
		&& (context.mInput.mFlags & SearchFlags::DontCreateChildForContent) == 0)
	{
		BeginChild();
	}

	const bool wasPressed = context.mIndexOfPressedItem == context.mInput.mEntries.size();

	if (wasPressed)
	{
		ProcessItemClickConsumption(context);
	}

	context.mInput.mEntries.emplace_back(
		Entry
		{
			false,
			0
		});
	context.mInput.mNames.Emplace(name);
	context.mInput.mBonuses.emplace_back();
	context.mDisplayFunctions.emplace_back(ItemFunctions{ std::move(display) });

	return wasPressed;
}

void CE::Search::TreeNode(std::string_view label)
{
	BeginCategory(label, [](std::string_view l) { return ImGui::TreeNode(l.data()); });
}

void CE::Search::TreePop()
{
	EndCategory([] { ImGui::TreePop(); });
}

bool CE::Search::Button(std::string_view label)
{
	return AddItem(label,
		[](std::string_view name)
		{
			return ImGui::MenuItem(name.data());
		}
	);
}

bool CE::Search::BeginCombo(std::string_view label, std::string_view previewValue, SearchFlags searchFlags)
{
	// Copied directly from ImGui::BeginCombo, but we use our own
	// popup instead. The default combo popup does not like our
	// usage of a child window, and it closes immediately when an
	// item is clicked, which does not give us the change to have
	// AddItem return true during the next frame.

	using namespace ImGui;

	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = GetCurrentWindow();

	ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
	g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
	if (window->SkipItems)
		return false;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label.data());

	const float arrow_size = GetFrameHeight();
	const ImVec2 label_size = CalcTextSize(label.data(), NULL, true);
	const float w = CalcItemWidth();
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
	const ImRect total_bb(bb.Min, bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &bb))
		return false;

	// Open on click
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held);
	const std::string popup_name = Format("{}##ComboPopUp", label);
	ImGuiID popup_id = ImGui::GetID(popup_name.data(), popup_name.data() + popup_name.size());

	bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
	if (pressed && !popup_open)
	{
		OpenPopupEx(popup_id, ImGuiPopupFlags_None);
		popup_open = true;
	}

	// Render shape
	const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	const float value_x2 = ImMax(bb.Min.x, bb.Max.x - arrow_size);
	RenderNavHighlight(bb, id);
	window->DrawList->AddRectFilled(bb.Min, ImVec2(value_x2, bb.Max.y), frame_col, style.FrameRounding, ImDrawFlags_RoundCornersLeft);

	ImU32 bg_col = GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	ImU32 text_col = GetColorU32(ImGuiCol_Text);
	window->DrawList->AddRectFilled(ImVec2(value_x2, bb.Min.y), bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
	if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x)
		RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, bb.Min.y + style.FramePadding.y), text_col, ImGuiDir_Down, 1.0f);

	RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);

	// Render preview and label
	if (previewValue.data() != NULL)
	{
		if (g.LogEnabled)
			LogSetNextTextDecoration("{", "}");
		RenderTextClipped(bb.Min + style.FramePadding, ImVec2(value_x2, bb.Max.y), previewValue.data(), NULL, NULL);
	}
	if (label_size.x > 0)
		RenderText(ImVec2(bb.Max.x + style.ItemInnerSpacing.x, bb.Min.y + style.FramePadding.y), label.data());

	if (!popup_open)
		return false;

	g.NextWindowData.Flags = backup_next_window_data_flags;

	ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());

	return BeginPopup(popup_name, searchFlags);
}

void CE::Search::EndCombo()
{
	End();
	ImGui::EndCombo();
}

bool CE::Search::BeginPopup(std::string_view name, SearchFlags searchFlags)
{
	ImGui::SetNextWindowSize(ImVec2{ -1.0f, 300.0f });

	if (!ImGui::BeginPopup(name.data()))
	{
		return false;
	}

	Begin(searchFlags);

	if (ImGui::IsWindowAppearing())
	{
		ImGui::SetKeyboardFocusHere(-1);
		sContextStack.top().get().mInput.mUserQuery.clear();
	}

	return true;
}

void CE::Search::EndPopup()
{
	SearchContext& context = sContextStack.top();

	if (context.mHasPressedItemBeenConsumed)
	{
		context.mHasPressedItemBeenConsumed = false;
		ImGui::CloseCurrentPopup();
	}

	End();
	ImGui::EndPopup();
}

void CE::Search::SetBonus(float bonus)
{
	sContextStack.top().get().mInput.mBonuses.back() = bonus;
}

const std::string& CE::Search::GetUserQuery()
{
	return sContextStack.top().get().mInput.mUserQuery;
}

namespace
{
	EntryAsNode::EntryAsNode(uint32& index, const Result& result):
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

	Result::~Result()
	{
		if (mThread.WasLaunched())
		{
			mThread.CancelOrJoin();
		}
	}

	bool operator==(const Entry& lhs, const Entry& rhs)
	{
		return lhs.mNumOfTotalChildren == rhs.mNumOfTotalChildren && lhs.mIsCategory == rhs.mIsCategory;
	}

	bool operator!=(const Entry& lhs, const Entry& rhs)
	{
		return lhs.mNumOfTotalChildren != rhs.mNumOfTotalChildren || lhs.mIsCategory != rhs.mIsCategory;
	}

	void BeginChild()
	{
		ImGui::BeginChild("SearchItems", ImGui::GetContentRegionAvail(), false, ImGuiWindowFlags_NavFlattened);
	}

	bool IsResultSafeToUse(const Result& oldResult, const Input& currentInput)
	{
		const Input& oldInput = oldResult.mInput;

		return oldInput.mNames == currentInput.mNames
			&& oldInput.mEntries == currentInput.mEntries;
	}

	bool IsResultUpToDate(const Result& oldResult, const Input& currentInput)
	{
		const Input& oldInput = oldResult.mInput;
		return oldInput.mUserQuery == currentInput.mUserQuery
			&& oldInput.mFlags == currentInput.mFlags
			&& oldInput.mBonuses == currentInput.mBonuses;
	}

	void AppendToDisplayOrder(const EntryAsNode& node, Result& result)
	{
		result.mOutput.mDisplayOrder.emplace_back(node.mIndex);

		for (const EntryAsNode& child : node.mChildren)
		{
			AppendToDisplayOrder(child, result);
		}

		if (result.mInput.mEntries[node.mIndex].mIsCategory)
		{
			result.mOutput.mDisplayOrder.emplace_back(node.mIndex | Output::sDisplayEndOfCategoryFlag);
		}
	}

	void PreprocessString(char* data, size_t count)
	{
		for (size_t i = 0; i < count; i++)
		{
			char& byte = data[i];
			byte = static_cast<char>(std::tolower(static_cast<unsigned char>(byte)));
		}
	}

	template<typename Scorer>
	void ApplyScoresUsingScorer(Result& result, std::string_view preprocessedQuery, double weight)
	{
		const CE::ManyStrings& names = result.mBuffers.mPreprocessedNames;
		std::vector<float>& scores = result.mBuffers.mScores;
		const Scorer scorer{ preprocessedQuery };

		const double factor = weight / 100.0;
		
		for (size_t i = 0; i < names.NumOfStrings(); i++)
		{
			scores[i] += static_cast<float>(scorer.similarity(names[i]) * factor);
		}
	}

	void GiveInitialScores(Result& result)
	{
		CE::ManyStrings& names = result.mBuffers.mPreprocessedNames;
		names = result.mInput.mNames;

		PreprocessString(names.Data(), names.SizeInBytes());

		std::string preprocessedQuery = result.mInput.mUserQuery;
		PreprocessString(preprocessedQuery.data(), preprocessedQuery.size());

		std::vector<float>& scores = result.mBuffers.mScores;
		scores = result.mInput.mBonuses;

		ApplyScoresUsingScorer<rapidfuzz::fuzz::CachedPartialTokenSortRatio<char>>(result, preprocessedQuery, .5);
		ApplyScoresUsingScorer<rapidfuzz::fuzz::CachedRatio<char>>(result, preprocessedQuery, .5);
	}

	void PropagateScoreToChildren(const std::vector<EntryAsNode>& nodes, Result& result)
	{
		for (const EntryAsNode& node : nodes)
		{
			const float nodeScore = result.mBuffers.mScores[node.mIndex];

			for (const EntryAsNode& child : node.mChildren)
			{
				float& childScore = result.mBuffers.mScores[child.mIndex];
				childScore = std::max(childScore, nodeScore);
			}

			PropagateScoreToChildren(node.mChildren, result);
		}
	}

	void PropagateScoreToParents(const std::vector<EntryAsNode>& nodes, Result& result)
	{
		for (const EntryAsNode& node : nodes)
		{
			PropagateScoreToParents(node.mChildren, result);

			float& nodeScore = result.mBuffers.mScores[node.mIndex];

			for (const EntryAsNode& child : node.mChildren)
			{
				const float childScore = result.mBuffers.mScores[child.mIndex];
				nodeScore = std::max(nodeScore, childScore);
			}
		}
	}

	float CalculateCutOff(const Result& result)
	{
		const std::vector<float>& scores = result.mBuffers.mScores;
		const float highestScore = *std::max_element(scores.begin(), scores.end());
		const float average = std::accumulate(scores.begin(), scores.end(), 0.0f) / static_cast<float>(scores.size());
		return CE::Math::lerp(average, highestScore, sFilterStrength);
	}

	void RemoveAllBelowCutOff(std::vector<EntryAsNode>& nodes, const Result& result, float cutOff)
	{
		nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
			[cutOff, &result](const EntryAsNode& node)
			{
				return result.mBuffers.mScores[node.mIndex] < cutOff;
			}), nodes.end());

		for (EntryAsNode& node : nodes)
		{
			RemoveAllBelowCutOff(node.mChildren, result, cutOff);
		}
	}

	void SortNodes(std::vector<EntryAsNode>& nodes, const Result& result)
	{
		std::stable_sort(nodes.begin(), nodes.end(),
			[&result](const EntryAsNode& lhs, const EntryAsNode& rhs)
			{
				return  result.mBuffers.mScores[lhs.mIndex] > result.mBuffers.mScores[rhs.mIndex];
			});

		for (EntryAsNode& node : nodes)
		{
			SortNodes(node.mChildren, result);
		}
	}

	[[maybe_unused]] std::string ConvertNodeTreeToString(const std::vector<EntryAsNode>& nodes, const Result& result, std::string& indentation)
	{
		std::string str{};

		for (const EntryAsNode& node : nodes)
		{
			str += CE::Format("{}{} = {}\n", indentation, result.mInput.mNames[node.mIndex], result.mBuffers.mScores[node.mIndex]);

			indentation.push_back('\t');
			str.append(ConvertNodeTreeToString(node.mChildren, result, indentation));
			indentation.pop_back();
		}

		return str;
	}

	[[maybe_unused]] void PrintNodeTree([[maybe_unused]] const std::vector<EntryAsNode>& nodes, [[maybe_unused]] const Result& result, [[maybe_unused]] std::string_view stage)
	{
		if constexpr (sDebugPrintingEnabled)
		{
			[[maybe_unused]] std::string indentation{};
			LOG(LogSearch, Verbose, "Stage: {}\nSearchTerm: {}\n{}\n", stage, result.mInput.mUserQuery, ConvertNodeTreeToString(nodes, result, indentation));
		}
	}

	void BringResultUpToDate(Result& result)
	{
		result.mOutput.mDisplayOrder.clear();

		const std::vector<Entry>& entries = result.mInput.mEntries;

		// I've never had this happen, and I've never tested with zero entries.
		// but because of how rare it is to want to search through zero entries,
		// we should try and avoid any unexpected edge cases when we have zero entries.
		if (entries.empty())
		{
			return;
		}

		std::vector<EntryAsNode>& nodes = result.mBuffers.mNodes;

		nodes.clear();

		for (uint32 i = 0; i < entries.size();)
		{
			nodes.emplace_back(i, result);
		}

		if (!result.mInput.mUserQuery.empty())
		{
			GiveInitialScores(result);
			PrintNodeTree(nodes, result, "Initial scores");

			SortNodes(nodes, result);
			PrintNodeTree(nodes, result, "First sorting pass");

			if ((result.mInput.mFlags & CE::Search::IgnoreParentScore) == 0)
			{
				PropagateScoreToChildren(nodes, result);
				PrintNodeTree(nodes, result, "Propagated to children");
			}

			PropagateScoreToParents(nodes, result);
			PrintNodeTree(nodes, result, "Propagated to parents");

			const float cutOff = CalculateCutOff(result);
			RemoveAllBelowCutOff(nodes, result, cutOff);
			PrintNodeTree(nodes, result, CE::Format("Removing scores below {}", cutOff));

			SortNodes(nodes, result);
			PrintNodeTree(nodes, result, "Second sorting pass");
		}

		for (const EntryAsNode& node : nodes)
		{
			AppendToDisplayOrder(node, result);
		}
	}

	void ProcessItemClickConsumption(SearchContext& context)
	{
		context.mIndexOfPressedItem = std::numeric_limits<uint32>::max();
		context.mHasPressedItemBeenConsumed = true;
	}

	void DisplayToUser(SearchContext& context)
	{
		ImGui::PushID(context.mInput.mUserQuery.empty());

		const std::vector<uint32>& displayOrder = context.mResults[context.mIndexOfLastValidResult].mOutput.mDisplayOrder;
		static constexpr uint32 endOfCatFlag = Output::sDisplayEndOfCategoryFlag;

		for (auto displayCommand = displayOrder.begin(); displayCommand != displayOrder.end(); ++displayCommand)
		{
			const uint32 index = *displayCommand & (~endOfCatFlag);

			const std::string_view name = context.mInput.mNames[index];
			const Entry& entry = context.mInput.mEntries[index];
			const std::variant<CategoryFunctions, ItemFunctions>& functions = context.mDisplayFunctions[index];

			if (!entry.mIsCategory)
			{
				const ItemFunctions& itemFunctions = std::get<ItemFunctions>(functions);

				if (itemFunctions.mOnDisplay
					&& (itemFunctions.mOnDisplay(name) || !context.mHasEnterBeenConsumed))
				{
					context.mIndexOfPressedItem = index;
					context.mHasPressedItemBeenConsumed = false;
					context.mHasEnterBeenConsumed = true;
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

			if (!catFunctions.mOnDisplayStart)
			{
				continue;
			}

			if (!context.mInput.mUserQuery.empty())
			{
				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			}

			// Open the tab
			if (catFunctions.mOnDisplayStart(name))
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
				const Entry& entryToSkip = context.mInput.mEntries[indexOfEntryToSkip];

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

		ImGui::PopID();
	}
}
