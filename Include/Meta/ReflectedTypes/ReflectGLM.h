#pragma once

template<>
struct Reflector<glm::vec2>
{
	static Engine::MetaType Reflect();
}; REFLECT_AT_START_UP(glmVec2, glm::vec2);

template<>
struct Reflector<glm::vec3>
{
	static Engine::MetaType Reflect();
}; REFLECT_AT_START_UP(glmVec3, glm::vec3);

template<>
struct Reflector<glm::vec4>
{
	static Engine::MetaType Reflect();
}; REFLECT_AT_START_UP(glmVec4, glm::vec4);

template<>
struct Reflector<glm::quat>
{
	static Engine::MetaType Reflect();
}; REFLECT_AT_START_UP(glmQuat, glm::quat);

template<>
struct Reflector<glm::mat4>
{
	static Engine::MetaType Reflect();
}; REFLECT_AT_START_UP(glmMat4, glm::mat4);

