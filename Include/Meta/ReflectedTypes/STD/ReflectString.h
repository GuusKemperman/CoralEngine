#pragma once

template<>
struct Reflector<std::string>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(stdString, std::string);
