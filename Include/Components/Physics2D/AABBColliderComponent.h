#pragma once
#include "Components/Pathfinding/Geometry2d.h"
#include "Meta/MetaReflect.h"

namespace Engine
{
	class TransformComponent;

	// Will completely ignore all rotations
	class AABBColliderComponent
	{
	public:
		TransformedAABB CreateTransformedCollider(const TransformComponent& transform) const;

		glm::vec2 mHalfExtends{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AABBColliderComponent);
	};

	using TransformedAABBColliderComponent = TransformedAABB;
}

template<>
struct Reflector<Engine::TransformedAABBColliderComponent>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(TransformedAABBColliderComponent, Engine::TransformedAABBColliderComponent);
