#pragma once
#include "Random.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

namespace CE
{
	template<typename ValueType>
	class WeightedRandomDistribution
	{
	public:
		bool operator==(const WeightedRandomDistribution& other) const { return mWeights == other.mWeights; }
		bool operator!=(const WeightedRandomDistribution& other) const { return mWeights != other.mWeights; }

		ValueType* GetNext()
		{
			float total{};

			for (const auto& [value, chance] : mWeights)
			{
				total += chance;
			}

			std::sort(mWeights.begin(), mWeights.end(),
				[](const auto& lhs, const auto& rhs)
				{
					return lhs.second < rhs.second;
				});

			const float randomNum = Random::Range(0.0f, total);

			float cumulative{};

			for (size_t i = 0; i < mWeights.size(); i++)
			{
				const float nextCumulative = cumulative + mWeights[i].second;

				if (randomNum >= cumulative
					&& randomNum <= nextCumulative)
				{
					return &mWeights[i].first;
				}

				cumulative = nextCumulative;
			}
			return nullptr;
		}

		std::vector<std::pair<ValueType, float>> mWeights{};
	};

	template<class Archive, typename T>
	void serialize(Archive& ar, WeightedRandomDistribution<T>& value)
	{
		ar(value.mWeights);
	}

}

template<typename T>
struct Reflector<CE::WeightedRandomDistribution<T>>
{
	static_assert(CE::sIsReflectable<T>, "Cannot reflect an optional of a type that is not reflected");

	static CE::MetaType Reflect()
	{
		using namespace CE;
		const MetaType& basedOnType = MetaManager::Get().GetType<T>();
		MetaType optType{ MetaType::T<WeightedRandomDistribution<T>>{}, Format("{} Weighted Distribution", basedOnType.GetName()) };

		optType.AddFunc(&WeightedRandomDistribution<T>::GetNext, "GetNext");

		ReflectFieldType<WeightedRandomDistribution<T>>(optType);

		return optType;
	}
	static constexpr bool sIsSpecialized = true;
};

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<typename T>, CE::WeightedRandomDistribution<T>, ImGui::Auto(var.mWeights, name); )
#endif

