#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"
#include "Utilities/Events.h"

namespace CE
{
	class Animation;
	class World;
	class Registry;

	struct OnAnimationFinish :
		EventType<OnTick>
	{
		OnAnimationFinish() :
			EventType("OnAnimationFinish")
		{
		}
	};
	/*
	* \brief Called when an animation finishes.
	* World& the world the animationRootComponent is in
	* entt::entity The entity with the animationRootComponent which has finished its animation.
	*/
	inline const OnAnimationFinish sOnAnimationFinish{};

	class AnimationRootComponent
	{
	public:
		void SwitchAnimation();
		/// <summary>
		/// Switch the animation and related values for the root and children of this components owner
		/// The timestamp will always be set when switching to a new animation
		/// </summary>
		void SwitchAnimation(Registry& reg, const AssetHandle<Animation>& animation, float timeStamp = 0.0f, float animationSpeed = 1.0f, float blendTime = 0.2f);

		void OnConstruct(World&, entt::entity owner);

		entt::entity mOwner{};

		float mCurrentTimeStamp{};
		float mCurrentAnimationSpeed{};
		AssetHandle<Animation> mCurrentAnimation{};

		float mWantedTimeStamp = 0.0f;
		float mWantedAnimationSpeed = 1.0f;
		float mWantedBlendTime = 0.2f;
		AssetHandle<Animation> mWantedAnimation{};

	private:
		void SwitchAnimationRecursive(Registry& reg, const entt::entity entity, const AssetHandle<Animation> animation, float timeStamp, float animationSpeed = 1.0f, float blendTime = 0.2f);

		friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(AnimationRootComponent);
	};
}
