#pragma once

namespace Engine::Internal
{
	struct Null {};
}

template<>
struct Reflector<Engine::Internal::Null>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(engineNull, Engine::Internal::Null);
