#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	/// <summary>
	/// A polygon-shaped collider for physics.
	/// </summary>
	class PolygonColliderComponent
	{
	public:
		/// <summary>
		/// The boundary vertices of the polygon in local coordinates,
		/// i.e. relative to the object's rotation and center of mass.
		/// </summary>
		std::vector<glm::vec2> mPoints = {};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PolygonColliderComponent);
	};
}
