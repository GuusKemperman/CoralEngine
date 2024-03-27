#pragma once
#include "Components/Pathfinding/Geometry2d.h"
#include "Meta/MetaReflect.h"

namespace Engine
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
struct Reflector<Engine::TransformedDiskColliderComponent>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(TransformedDiskColliderComponent, Engine::TransformedDiskColliderComponent);