#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class Animation;
	class World;
	class Registry;

	class AnimationRootComponent
	{
	public:
		void SwitchAnimation();
		void SwitchAnimation(Registry& reg, const AssetHandle<Animation>& animation, float timeStamp, float animationSpeed = 1.0f, float blendSpeed = 1.0f);

		void OnConstruct(World&, entt::entity owner);

		entt::entity mOwner{};

		float mWantedTimeStamp = 0.0f;
		float mWantedAnimationSpeed = 1.0f;
		float mWantedBlendSpeed = 1.0f;
		AssetHandle<Animation> mWantedAnimation{};

	private:
		void SwitchAnimationRecursive(Registry& reg, const entt::entity entity, const AssetHandle<Animation> animation, float timeStamp, float animationSpeed = 1.0f, float blendSpeed = 1.0f);

		friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(AnimationRootComponent);
	};
}