#pragma once

template<>
struct Reflector<entt::entity>
{
	static Engine::MetaType Reflect();
}; REFLECT_AT_START_UP(enttEntity, entt::entity);
