#pragma once
#ifdef EDITOR
#include "rapidfuzz/rapidfuzz_all.hpp"

#include "Meta/MetaManager.h"
#include "Meta/MetaType.h"
#include "Meta/MetaReflect.h"
#include "Assets/Asset.h"
#include "Core/AssetManager.h"
#include "Utilities/EnumString.h"

namespace CE::Search
{
	template<typename T, typename T2 = void>
	struct MovableTypeHelper
	{
		using Type = T;
	};

	template<>
	struct MovableTypeHelper<MetaType>
	{
		using Type = std::reference_wrapper<const MetaType>;
	};

	template<typename T>
	struct MovableTypeHelper<T, std::enable_if_t<sIsAssetType<T>>>
	{
		using Type = WeakAsset<T>;
	};

	/*
	 * A specialization can be provided to MovableTypeHelper.
	 *
	 * DisplayDropDownWithSearchBar<Asset> will for example return
	 * an std::optional<WeakAsset<Asset>>, since a specialization
	 * for MovableTypeHelper is in place.
	 */
	template<typename T>
	using MovableType = typename MovableTypeHelper<T>::Type;

	template<typename... Types>
	struct ChoiceValueTypeHelper
	{
		using Type = std::variant<MovableType<Types>...>;
	};

	template<typename T>
	struct ChoiceValueTypeHelper<T>
	{
		using Type = MovableType<T>;
	};

	/*
	* If multiple types are provided, a variant is used. Otherwise it is just the lone value.
	*
	* This allows for searching multiple things at once, a searchbar that can be used to lookup
	* assets AND types.
	*
	* Example:
	*
	*   // Seperate:
	*   std::optional<WeakAsset<Asset>> asset = DisplayDropDownWithSearchBar<Asset>();
	*   std::optional<std::reference_wrapper<const MetaType>> type = DisplayDropDownWithSearchBar<MetaType>();
	*
	*   // Combined:
	*   std::optional<std::variant<WeakAsset<Asset>, std::reference_wrapper<const MetaType>>> variant = DisplayDropDownWithSearchBar<Asset, MetaType>();
	*/
	template<typename... Types>
	using ChoiceValueType = typename ChoiceValueTypeHelper<Types...>::Type;

	template<typename... Types>
	struct Choice
	{
		template<typename... Args>
		Choice(std::string_view name, Args&&... args) :
			mValue(std::forward<Args>(args)...),
			mName(std::move(name))
		{
			
		}

		ChoiceValueType<Types...> mValue;
		std::string mName{};
		double mSimilarity{};
	};

	template<typename... Types>
	using Choices = std::vector<Choice<Types...>>;

	template<typename T, typename T2 = void>
	struct SearchBarOptionsCollector;

	template<typename... Types>
	using FilterParam = const ChoiceValueType<Types...>&;

	static constexpr double sDefaultCutOff = 45.0;
	static constexpr size_t sDefaultMaxNumOfChoices = 25;
	static constexpr std::string_view sDefaultLabel = ICON_FA_SEARCH "##SearchBar";
	static constexpr std::string_view sDefaultHint = "Search";

	using CachedScorer = rapidfuzz::fuzz::CachedPartialTokenSortRatio<char>;

	/*
	 * Will collect all the items of the specified type. The provided filter should return true if you
	 * want the item to be included in the search.
	 *
	 * The returned value is not be sorted in any way.
	 *
	 * Example:
	 * 		Search::Choices<MetaType> choices = Search::CollectChoices<MetaType>(
	 * 		[&classesThatCannotBeAdded](const MetaType& type)
	 *		{
	 *          // Choices will now only contain metatypes that have the Props::sComponentTag property.
	 *			return type.GetProperties().Has(Props::sComponentTag);
	 *		});
	 */
	template<typename... Types>
	Choices<Types...> CollectChoices();

	template<typename... Types, typename FilterType>
	Choices<Types...> CollectChoices(const FilterType& filter);

	/*
	 * Will use the fuzzy search algorithm to eliminate the choices that differ too much from
	 * query.
	 *
	 * If an empty query is provided, no choices are eliminated.
	 *
	 * cutOff must be between 0.0 and 100.0. A higher cutOff means more choices are eliminated.
	 *
	 * The returned value is sorted from most matching first, and in alphabetical order second.
	 *
	 * Example:
	 *	std::string searchFor = "CubeAsset"
	 * 	Search::Choices<Asset> assetsThatMatchSearch = Search::CollectChoices<Asset>();
     *  Search::EraseChoicesThatDoNotMatch(searchFor, assetsThatMatchSearch);
	 */
	template<typename... Types>
	void EraseChoicesThatDoNotMatch(std::string_view query,
		Choices<Types...>& choices = CollectChoices<Types...>(),
		double cutOff = sDefaultCutOff,
		size_t maxNumOfChoices = sDefaultMaxNumOfChoices);

	/*
	 * Will display a search bar and return the string that the user has provided as input.
	 *
	 * Once the user deselects the search bar, the string is reset.
	 */
	std::string DisplaySearchBar(std::string_view label = sDefaultLabel, std::string_view hint = sDefaultHint);

	/*
	 * Will display a search bar and return the string that the user has provided as input.
	 */
	void DisplaySearchBar(std::string& searchTerm, std::string_view label = sDefaultLabel, std::string_view hint = sDefaultHint);

	/*
	 * Will display a search bar, and a button for each of the choices. The return value is the choice selected
	 * by the user.
	 */
	template<typename... Types>
	std::optional<ChoiceValueType<Types...>> DisplaySearchBar(Choices<Types...>& choices = CollectChoices<Types...>(),
		std::string_view label = sDefaultLabel,
		std::string_view hint = sDefaultHint);

	/*
	 * The search bar is initially not visible. The user can click on a drop down menu, which contains the
	 * search bar and all of the options. The return value is the choice selected
	 * by the user.
	 *
	 * Example:
	 *  // You can obviously use auto here as well, but just for the sake of clarity.
	 * 	std::optional<std::reference_wrapper<const MetaType>> selectedType = Search::DisplayDropDownWithSearchBar<MetaType>(
	 * 		"Type: ",
	 * 		GetNameOfType(mParam.mTypeTraits.mStrippedTypeId).c_str(),
	 *		[](const MetaType& type)
	 *		{
	 *			// Will only search for types that are referencable by script.
	 *			return CanTypeBeReferencedInScripts(type);
	 *		});
	 */
	template<typename... Types>
	std::optional<ChoiceValueType<Types...>> DisplayDropDownWithSearchBar(std::string_view label, std::string_view hint);
	template<typename... Types, typename FilterType>
	std::optional<ChoiceValueType<Types...>> DisplayDropDownWithSearchBar(std::string_view label, std::string_view hint, const FilterType& filter);

	bool BeginSearchCombo(std::string_view label, std::string_view hint);

	void EndSearchCombo(bool wasItemSelected);

	//**************************//
	//		Implementation		//
	//**************************//

	namespace Internal
	{
		template<typename T, typename... Types, typename FilterType>
		void AddOptions(Choices<Types...>& scores, const FilterType& filter);
	}

	template<>
	struct SearchBarOptionsCollector<MetaType>
	{
		template<typename... Types, typename FilterType>
		static void AddOptions(Choices<Types...>& insertInto, const FilterType& filter);
	};

	template<typename AssetType>
	struct SearchBarOptionsCollector<AssetType, std::enable_if_t<sIsAssetType<AssetType>>>
	{
		template<typename... Types, typename FilterType>
		static void AddOptions(Choices<Types...>& insertInto, const FilterType& filter)
		{
			for (const WeakAsset<Asset>& asset : AssetManager::Get().GetAllAssets())
			{
				const MetaType& assetClass = asset.GetAssetClass();

				if (!assetClass.IsDerivedFrom<AssetType>())
				{
					continue;
				}

				WeakAsset<AssetType> casted = WeakAssetStaticCast<AssetType>(asset);

				if (filter(casted))
				{
					insertInto.emplace_back(asset.GetName(), casted);
				}
			}
		}
	};

	// Enums
	template<typename EnumType>
	struct SearchBarOptionsCollector<EnumType, std::enable_if_t<sIsEnumReflected<EnumType>>>
	{
		template<typename... Types, typename FilterType>
		static void AddOptions(Choices<Types...>& insertInto, const FilterType& filter)
		{
			for (const auto& [enumValue, enumName] : sEnumStringPairs<EnumType>)
			{
				if (filter(enumValue))
				{
					insertInto.emplace_back(enumName, enumValue);
				}
			}
		}
	};

	template<typename... Types>
	bool DefaultFilter(FilterParam<Types...>) { return true; }
	

	template<typename... Types>
	constexpr bool operator<(const Choice<Types...>& lhs, const Choice<Types...>& rhs)
	{
		if (lhs.mSimilarity == rhs.mSimilarity)
		{
			return lhs.mName < rhs.mName;
		}
		return lhs.mSimilarity > rhs.mSimilarity;
	}

	template <typename T, typename ... Types, typename FilterType>
	void Internal::AddOptions(Choices<Types...>& scores, const FilterType& filter)
	{
		SearchBarOptionsCollector<T>::AddOptions(scores, filter);
	}

	template <typename ... Types, typename FilterType>
	void SearchBarOptionsCollector<MetaType>::AddOptions(Choices<Types...>& insertInto, const FilterType& filter)
	{
		for (const MetaType& type : MetaManager::Get().EachType())
		{
			if (filter(type))
			{
				insertInto.emplace_back(type.GetName(), type);
			}
		}
	}

	template<typename... Types, typename FilterType>
	Choices<Types...> CollectChoices(const FilterType& filter)
	{
		static_assert(std::is_invocable_v<FilterType, FilterParam<Types...>>, "Filter cannot be invoked with the associated arguments");
		static_assert(std::is_same_v<bool, std::invoke_result_t<FilterType, FilterParam<Types...>>>, "Filter does not return boolean");

		Choices<Types...> choices{};
		(Internal::AddOptions<Types>(choices, filter), ...);
		return choices;
	}

	template<typename... Types>
	Choices<Types...> CollectChoices()
	{
		return CollectChoices<Types...>(&DefaultFilter<Types...>);
	}

	template<typename... Types>
	void EraseChoicesThatDoNotMatch(std::string_view query,
		Choices<Types...>& choices,
		double cutOff,
		size_t maxNumOfChoices)
	{
		if (!query.empty())
		{
			CachedScorer scorer(query);

			//#pragma omp parallel for
			for (size_t i = 0; i < choices.size(); ++i)
			{
				Choice<Types...>& choice = choices[i];

				choice.mSimilarity = scorer.similarity(choice.mName, cutOff);
			}

			std::sort(choices.begin(), choices.end());

			size_t minNumOfChoicesToErase = choices.size() > maxNumOfChoices ? choices.size() - maxNumOfChoices : 0;

			auto discardFrom = std::find_if(choices.rbegin() + minNumOfChoicesToErase, choices.rend(),
				[&cutOff](const Choice<Types...>& choice)
				{
					return choice.mSimilarity > cutOff;
				});

			choices.erase(discardFrom.base(), choices.end());
		}
		else
		{
			std::sort(choices.begin(), choices.end());
		}
	}

	template<typename... Types>
	std::optional<ChoiceValueType<Types...>> DisplaySearchBar(Choices<Types...>& choices,
		std::string_view label,
		std::string_view hint)
	{
		const std::string query = DisplaySearchBar(label, hint);
		const bool isTyping = ImGui::IsItemFocused();

		EraseChoicesThatDoNotMatch<Types...>(query, choices);

		if (isTyping
			&& !choices.empty()
			&& ImGui::IsKeyPressed(ImGuiKey_Enter))
		{
			return choices[0].mValue;
		}

		using Choice = Choice<Types...>;

		for (Choice& choice : choices)
		{
			if (ImGui::Button(choice.mName.c_str())
				|| ImGui::IsItemClicked()) // If we click on the button, the search term is reset, the button may move, 
										// and we won't be hovering over the button when the mouse button is released.
										// So we activate OnClick and not OnRelease
			{
				return choice.mValue;
			}
		}

		return std::nullopt;
	}

	template<typename... Types, typename FilterType>
	std::optional<ChoiceValueType<Types...>> DisplayDropDownWithSearchBar(std::string_view label, std::string_view hint,
		const FilterType& filter)
	{
		using Choices = Choices<Types...>;

		if (!BeginSearchCombo(label, hint))
		{
			return std::nullopt;
		}

		Choices choices = CollectChoices<Types...>(filter);
		auto returnValue = DisplaySearchBar<Types...>(choices);

		EndSearchCombo(returnValue.has_value());

		return returnValue;
	}

	template<typename... Types>
	std::optional<ChoiceValueType<Types...>> DisplayDropDownWithSearchBar(std::string_view label, std::string_view hint)
	{
		return DisplayDropDownWithSearchBar<Types...>(label, hint, &DefaultFilter<Types...>);
	}
}
#endif // EDITOR