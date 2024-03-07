#pragma once
#include "Meta/MetaTypeFilter.h"
#include "Meta/MetaReflect.h"

namespace Engine
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
struct Reflector<Engine::ComponentFilter>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(EngineComponentFilter, Engine::ComponentFilter);
