#pragma once

template<>
struct Reflector<entt::entity>
{
	static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(enttEntity, entt::entity);
