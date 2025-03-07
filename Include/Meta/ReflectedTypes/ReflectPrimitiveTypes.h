#pragma once

template<>
struct Reflector<int32>
{
	static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(int32);

template<>
struct Reflector<float32>
{
	static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(float32);

template<>
struct Reflector<bool>
{
	static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(bool);

template<>
struct Reflector<uint32>
{
	static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(uint32);

