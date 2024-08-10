#pragma once
#include "Meta/MetaReflect.h"
#include "BasicDataTypes/Colors/LinearColor.h"

namespace CE
{
	class World;

	class PointLightComponent
	{
	public:
		LinearColor mColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		float mRange = 5.0f;

		void OnDrawGizmos(World& world, entt::entity owner) const;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PointLightComponent);
	};
}
