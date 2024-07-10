#pragma once
#include "Utilities/Imgui/ImguiInspect.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaType.h"

namespace Engine
{
	template<typename Filter>
	class MetaTypeFilter
	{
	public:
		MetaTypeFilter() = default;
		MetaTypeFilter(const MetaType* type);

		MetaTypeFilter& operator=(const MetaType* type);

		bool operator==(nullptr_t) const { return mValue == nullptr; };
		bool operator==(const MetaTypeFilter& other) const { return mValue == other.mValue; };
		bool operator!=(const MetaTypeFilter& other) const { return mValue != other.mValue; };

		operator const MetaType*() const { return Get(); }

		static bool IsTypeValid(const MetaType& type)
		{
			Filter f{};
			return f(type);
		}

		const MetaType* Get() const { return mValue; }

	private:
		const MetaType* mValue{};
	};

	template<class Archive, typename Filter>
	void save(Archive& ar, const MetaTypeFilter<Filter>& value)
	{
		save(ar, value.Get() == nullptr ? 0 : value.Get()->GetTypeId());
	}

	template<class Archive, typename Filter>
	void load(Archive& ar, MetaTypeFilter<Filter>& value)
	{
		TypeId typeId{};
		load(ar, typeId);

		if (typeId == 0)
		{
			return;
		}

		const MetaType* type = MetaManager::Get().TryGetType(typeId);

		if (type == nullptr)
		{
			LOG(LogMeta, Error, "Could not deserialize type, no type exists anymore with type id {}", typeId);
			return;
		}

		if (!MetaTypeFilter<Filter>::IsTypeValid(*type))
		{
			LOG(LogMeta, Error, "Could not deserialize type, type {} no longer matches filter {}", type->GetName(), MakeTypeName<Filter>())
				return;
		}

		value = type;
	}

	template <typename Filter>
	MetaTypeFilter<Filter>::MetaTypeFilter(const MetaType* type) :
		mValue(type)
	{
		ASSERT(type == nullptr || IsTypeValid(*type));
	}

	template <typename Filter>
	MetaTypeFilter<Filter>& MetaTypeFilter<Filter>::operator=(const MetaType* type)
	{
		mValue = type;
		ASSERT(type == nullptr || IsTypeValid(*type));
		return *this;
	}
}


#ifdef EDITOR
IMGUI_AUTO_DEFINE_BEGIN(template<typename Filter>, Engine::MetaTypeFilter<Filter>)
using namespace Engine;

std::optional<std::reference_wrapper<const MetaType>> selectedType = Search::DisplayDropDownWithSearchBar<MetaType>(name, var.Get() == nullptr ? "None" : var.Get()->GetName(),
		[](const MetaType& type)
		{
			return MetaTypeFilter<Filter>::IsTypeValid(type);
		});

if (selectedType.has_value())
{
	var = &(selectedType->get());
}
IMGUI_AUTO_DEFINE_END
#endif // EDITOR