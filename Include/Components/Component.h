#pragma once
#include "Meta/MetaTypeFilter.h"
#include "Meta/MetaReflect.h"

namespace Engine
{
	// This can be specialized so that everytime you call Registry::AddComponent<T>,
	// the owning entity is automatically passed to the constructor of the component
	// See TransformComponent.h for example
	template<typename T>
	struct AlwaysPassComponentOwnerAsFirstArgumentOfConstructor 
	{
		static constexpr bool sValue = false;
	};

	/*
	Some component require extra steps during serialization. This can be done by adding the following function:

	FUNCTION()
	void OnSerialize(BinaryGSONObject& serializeTo, const entt::entity owner, const World& world) const;

	TryGetComponentOnSerialize returns a pointer to this function, if it exists.
	*/
	constexpr Name sComponentCustomSerializeFuncName = "OnSerialize"_Name;

	/*
	Some component require extra steps during deserialization. This can be done by adding the following function:

	FUNCTION()
	void OnSerialize(BinaryGSONObject& serializeTo, const entt::entity owner, const World& world) const;

	TryGetComponentOnDeserialize returns a pointer to this function, if it exists.
	*/
	constexpr Name sComponentCustomDeserializeFuncName = "OnDeserialize"_Name;

	/*
	Some components may have a custom OnInspect function.

	FUNCTION()
	static void OnInspect(Registry& registry, const std::vector<entt::entity>& entities);
	*/
	constexpr Name sComponentCustomOnInspectFuncName = "OnInspect"_Name;

	namespace Internal
	{
		struct ComponentTypeFilterFunctor
		{
			bool operator()(const MetaType& type) const;
		};
	}
	using ComponentFilter = MetaTypeFilter<Internal::ComponentTypeFilterFunctor>;
}


template<>
struct Reflector<Engine::ComponentFilter>
{
	static Engine::MetaType Reflect();
}; REFLECT_AT_START_UP(EngineComponentFilter, Engine::ComponentFilter);
