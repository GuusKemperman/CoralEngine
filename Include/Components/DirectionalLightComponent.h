#pragma once
#include "Meta/MetaReflect.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"

namespace Engine
{
	class World;

	class DirectionalLightComponent
	{
	public:
		glm::vec3 mColor = { 1.0f, 1.0f, 1.0f };
		float mIntensity = 1.0f;
		float mExtent = 32.f;
		bool mCastsShadows = true;

		void OnDrawGizmos(World& world, entt::entity owner) const;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(DirectionalLightComponent);
	};
}

