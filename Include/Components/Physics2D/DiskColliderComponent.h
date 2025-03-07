#pragma once
#include "Utilities/Geometry2d.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class TransformComponent;

	/// <summary>
	/// A disk-shaped collider for physics.
	/// </summary>
	class DiskColliderComponent
	{
	public:
		TransformedDisk CreateTransformedCollider(const TransformComponent& transform) const;

		float mRadius = 1.f;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(DiskColliderComponent);
	};

	using TransformedDiskColliderComponent = TransformedDisk;
}

template<>
struct Reflector<CE::TransformedDiskColliderComponent>
{
	static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(TransformedDiskColliderComponent, CE::TransformedDiskColliderComponent);