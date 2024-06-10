#include "Precomp.h"
#include "Components/AnimationRootComponent.h"

#include "Assets/Animation/Animation.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/TransformComponent.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::AnimationRootComponent::SwitchAnimationRecursive(Registry& reg, const entt::entity entity, const AssetHandle<Animation> animation, float timeStamp, float animationSpeed, float blendTime)
{
	auto skinnedMesh = reg.TryGet<SkinnedMeshComponent>(entity);

	if (skinnedMesh != nullptr)
	{
		if (skinnedMesh->mAnimation != animation)
		{
			if (skinnedMesh->mAnimation != nullptr)
			{
				skinnedMesh->mPreviousAnimation = skinnedMesh->mAnimation;
				skinnedMesh->mPrevAnimTime = skinnedMesh->mCurrentTime;
				skinnedMesh->mBlendWeight = 0.0f;
			}
			skinnedMesh->mAnimation = animation;
		}
		skinnedMesh->mCurrentTime = timeStamp;
		skinnedMesh->mAnimationSpeed = animationSpeed;
		skinnedMesh->mBlendTime = blendTime;
	}

	auto transform = reg.TryGet<TransformComponent>(entity);

	if (transform == nullptr)
	{
		return;
	}

	for (const TransformComponent& child : transform->GetChildren())
	{
		SwitchAnimationRecursive(reg, child.GetOwner(), animation, timeStamp, animationSpeed, blendTime);
	}
}

void CE::AnimationRootComponent::OnConstruct(World&, entt::entity owner)
{
	mOwner = owner;
}

void CE::AnimationRootComponent::SwitchAnimation()
{
	World* world = World::TryGetWorldAtTopOfStack();
	ASSERT(world != nullptr);

	mCurrentTimeStamp = mWantedTimeStamp;
	mCurrentAnimationSpeed = mWantedAnimationSpeed;
	mCurrentAnimation = mWantedAnimation;

	SwitchAnimationRecursive(world->GetRegistry(), mOwner, mWantedAnimation, mWantedTimeStamp, mWantedAnimationSpeed, mWantedBlendTime);
}

void CE::AnimationRootComponent::SwitchAnimation(Registry& reg, const AssetHandle<Animation>& animation, float timeStamp, bool changeTimeStamp, float animationSpeed, float blendTime)
{
	if (animation == mWantedAnimation)
	{
		mCurrentAnimation = animation;

		mWantedAnimationSpeed = animationSpeed;
		mCurrentAnimationSpeed = mWantedAnimationSpeed;
		
		if (changeTimeStamp)
		{
			mWantedTimeStamp = timeStamp;
			mCurrentTimeStamp = mWantedTimeStamp;
		}

		SwitchAnimationRecursive(reg, mOwner, mCurrentAnimation, mCurrentTimeStamp, mCurrentAnimationSpeed);
	
		return;
	}

	mWantedAnimation = animation;
	mWantedTimeStamp = timeStamp;
	mWantedAnimationSpeed = animationSpeed;
	mWantedBlendTime = blendTime;

	mCurrentTimeStamp = mWantedTimeStamp;
	mCurrentAnimationSpeed = mWantedAnimationSpeed;
	mCurrentAnimation = mWantedAnimation;

	SwitchAnimationRecursive(reg, mOwner, mWantedAnimation, mWantedTimeStamp, mWantedAnimationSpeed, mWantedBlendTime);
}

CE::MetaType CE::AnimationRootComponent::Reflect()
{
	auto type = MetaType{MetaType::T<AnimationRootComponent>{}, "AnimationRootComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	type.AddField(&AnimationRootComponent::mWantedAnimation, "mWantedAnimation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&AnimationRootComponent::mWantedTimeStamp, "mWantedTimeStamp").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&AnimationRootComponent::mWantedAnimationSpeed, "mWantedAnimationSpeed").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&AnimationRootComponent::mWantedBlendTime, "mWantedBlendTime").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc(static_cast<void (AnimationRootComponent::*)()>(&AnimationRootComponent::SwitchAnimation), "SwitchAnimationEditor").GetProperties().Add(Props::sCallFromEditorTag);

	type.AddFunc([](AnimationRootComponent& animationRoot, const AssetHandle<Animation>& animation, float timeStamp, float animationSpeed, float blendTime)
		{
			if (animation == nullptr)
			{
				LOG(LogWorld, Warning, "Attempted to set NULL animation.");
				return;
			}

			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			animationRoot.SwitchAnimation(world->GetRegistry(), animation, timeStamp, false, animationSpeed, blendTime);

		}, "SwitchAnimation", MetaFunc::ExplicitParams<AnimationRootComponent&,
		const AssetHandle<Animation>&, float, float, float>{}, "AnimationRootComponent", "Animation", "Time Stamp", "Animation Speed", "Blend Time").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddField(&AnimationRootComponent::mCurrentAnimation, "mCurrentAnimation").GetProperties().Add(Props::sIsEditorReadOnlyTag).Add(Props::sIsScriptReadOnlyTag);
	type.AddField(&AnimationRootComponent::mCurrentTimeStamp, "mCurrentTimeStamp").GetProperties().Add(Props::sIsEditorReadOnlyTag).Add(Props::sIsScriptReadOnlyTag);
	type.AddField(&AnimationRootComponent::mCurrentAnimationSpeed, "mCurrentAnimationSpeed").GetProperties().Add(Props::sIsEditorReadOnlyTag).Add(Props::sIsScriptReadOnlyTag);

	BindEvent(type, sConstructEvent, &AnimationRootComponent::OnConstruct);

	ReflectComponentType<AnimationRootComponent>(type);
	return type;
}
