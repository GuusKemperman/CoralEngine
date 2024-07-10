#pragma once

namespace CE::Internal
{
	struct Null {};
}

template<>
struct Reflector<CE::Internal::Null>
{
	static CE::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(engineNull, CE::Internal::Null);
