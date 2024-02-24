#pragma once
#include "Meta/Fwd/MetaTypeFilterFwd.h"

#include "Utilities/Imgui/ImguiInspect.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaType.h"

template <typename Filter>
Engine::MetaTypeFilter<Filter>::MetaTypeFilter(const MetaType* type) :
	mValue(type)
{
	ASSERT(type == nullptr || IsTypeValid(*type));
}

template <typename Filter>
Engine::MetaTypeFilter<Filter>& Engine::MetaTypeFilter<Filter>::operator=(const MetaType* type)
{
	mValue = type;
	ASSERT(type == nullptr || IsTypeValid(*type));
	return *this;
}

template <typename Filter>
bool Engine::MetaTypeFilter<Filter>::IsTypeValid(const MetaType& type)
{
	Filter f{};
	return f(type);
}

template<class Archive, typename Filter>
void Engine::save(Archive& ar, const MetaTypeFilter<Filter>& value)
{
	save(ar, value.Get() == nullptr ? 0 : value.Get()->GetTypeId());
}

template<class Archive, typename Filter>
void Engine::load(Archive& ar, MetaTypeFilter<Filter>& value)
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