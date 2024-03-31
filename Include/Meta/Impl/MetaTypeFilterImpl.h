#pragma once
#include "Meta/Fwd/MetaTypeFilterFwd.h"

#include "Utilities/Imgui/ImguiInspect.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaType.h"

template <typename Filter>
CE::MetaTypeFilter<Filter>::MetaTypeFilter(const MetaType* type) :
	mValue(type)
{
	ASSERT(type == nullptr || IsTypeValid(*type));
}

template <typename Filter>
CE::MetaTypeFilter<Filter>& CE::MetaTypeFilter<Filter>::operator=(const MetaType* type)
{
	mValue = type;
	ASSERT(type == nullptr || IsTypeValid(*type));
	return *this;
}

template <typename Filter>
bool CE::MetaTypeFilter<Filter>::IsTypeValid(const MetaType& type)
{
	Filter f{};
	return f(type);
}

template<class Archive, typename Filter>
void CE::save(Archive& ar, const MetaTypeFilter<Filter>& value)
{
	// We serialize a dummy, in the past we serialized
	// the typeId. (What a fool I was).
	// Since that's obviously not a very platform-agnostic way to serialize types,
	// we instead serialize the typename now, and the dummy for some ABI
	// compatibility reasons. We wouldnt want to accidentally interpret a
	// number as a string.
	static constexpr TypeId dummy = std::numeric_limits<TypeId>::max();
	ar(dummy, value.Get() == nullptr ? std::string{} : value.Get()->GetName());
}

template<class Archive, typename Filter>
void CE::load(Archive& ar, MetaTypeFilter<Filter>& value)
{
	TypeId typeId{};
	load(ar, typeId);

	const bool isNewFormat = typeId == std::numeric_limits<TypeId>::max();
	const MetaType* type{};

	if (isNewFormat)
	{
		std::string typeName{};
		load(ar, typeName);

		if (typeName.empty())
		{
			return;
		}

		type = MetaManager::Get().TryGetType(typeName);

		if (type == nullptr)
		{
			LOG(LogMeta, Error, "Could not deserialize type, {} no longer exists", typeName);
			return;
		}
	}
	else
	{
		if (typeId == 0)
		{
			return;
		}

		type = MetaManager::Get().TryGetType(typeId);

		if (type == nullptr)
		{
			LOG(LogMeta, Error, "Could not deserialize type, type with typeId {} no longer exists", typeId);
			return;
		}
	}

	if (!MetaTypeFilter<Filter>::IsTypeValid(*type))
	{
		LOG(LogMeta, Error, "Could not deserialize type, type {} no longer matches filter {}", type->GetName(), MakeTypeName<Filter>());
		return;
	}

	value = type;
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_BEGIN(template<typename Filter>, CE::MetaTypeFilter<Filter>)
using namespace CE;

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