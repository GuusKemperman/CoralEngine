#pragma once

namespace CE::Internal
{
	struct Null {};
}

template<>
struct Reflector<CE::Internal::Null>
{
	static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(engineNull, CE::Internal::Null);
