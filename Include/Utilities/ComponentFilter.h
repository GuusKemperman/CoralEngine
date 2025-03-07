#pragma once
#include "Meta/MetaTypeFilter.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	namespace Internal
	{
		struct ComponentTypeFilterFunctor
		{
			bool operator()(const MetaType& type) const;
		};
	}
	using ComponentFilter = MetaTypeFilter<Internal::ComponentTypeFilterFunctor>;
}

template<>
struct Reflector<CE::ComponentFilter>
{
	static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(EngineComponentFilter, CE::ComponentFilter);
