#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	/// <summary>
	/// A disk-shaped collider for physics.
	/// </summary>
	class DiskColliderComponent
	{
	public:
		float mRadius = 1.f;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(DiskColliderComponent);
	};
}
