#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class Animation;
	class World;
	class Registry;

	class AnimationRootComponent
	{
	public:
		void SwitchAnimation();
		void SwitchAnimation(Registry& reg, const std::shared_ptr<const Animation>& animation, float timeStamp);

		void OnConstruct(World&, entt::entity owner);

		entt::entity mOwner{};

		float mWantedTimeStamp = 0.0f;
		std::shared_ptr<const Animation> mWantedAnimation{};

	private:
		void SwitchAnimationRecursive(Registry& reg, const entt::entity entity, const std::shared_ptr<const Animation> animation, float timeStamp);

		friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(AnimationRootComponent);
	};
}