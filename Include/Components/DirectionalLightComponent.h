#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;
	class TransformComponent;

	class DirectionalLightComponent
	{
	public:
		glm::vec3 mColor = { 1.0f, 1.0f, 1.0f };
		float mIntensity = 1.0f;
		bool mCastShadows = false;
		float mShadowExtent = 100.0f;
		float mShadowNearFar = 5000.0f;
		float mShadowBias = 0.00015f;

		void OnDrawGizmos(World& world, entt::entity owner) const;
		glm::mat4 GetShadowProjection() const;
		glm::mat4 GetShadowView(const World& world, const TransformComponent& transform) const;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(DirectionalLightComponent);
	};
}

