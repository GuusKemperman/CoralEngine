#pragma once
#include "Utilities/Geometry2d.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class TransformComponent;

	/// <summary>
	/// A polygon-shaped collider for physics.
	/// </summary>
	class PolygonColliderComponent
	{
	public:
		TransformedPolygon CreateTransformedCollider(const TransformComponent& transform) const;

		/// <summary>
		/// The boundary vertices of the polygon in local coordinates,
		/// i.e. relative to the object's rotation and center of mass.
		/// </summary>
		PolygonPoints mPoints{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PolygonColliderComponent);
	};

	using TransformedPolygonColliderComponent = TransformedPolygon;
}

template<>
struct Reflector<CE::TransformedPolygonColliderComponent>
{
	static CE::MetaType Reflect();
}; REFLECT_AT_START_UP(TransformedPolygonColliderComponent, CE::TransformedPolygonColliderComponent);