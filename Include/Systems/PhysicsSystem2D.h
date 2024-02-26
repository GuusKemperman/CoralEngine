#pragma once
#include "Systems/System.h"

namespace Engine
{
	class PhysicsSystem2D final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

	private:
		void UpdateTransforms(World& world, float dt);
		void DebugDrawing(World& world);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PhysicsSystem2D);
	};
}
