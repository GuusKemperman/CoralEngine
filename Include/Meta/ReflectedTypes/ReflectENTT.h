#pragma once

template<>
struct Reflector<entt::entity>
{
	static CE::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(enttEntity, entt::entity);
