#pragma once
#include "Utilities/Geometry2d.h"
#include "Meta/MetaReflect.h"

namespace CE
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
struct Reflector<CE::TransformedAABBColliderComponent>
{
	static CE::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(TransformedAABBColliderComponent, CE::TransformedAABBColliderComponent);
