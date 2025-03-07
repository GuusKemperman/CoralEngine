#pragma once

template<>
struct Reflector<std::string>
{
	static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(stdString, std::string);
