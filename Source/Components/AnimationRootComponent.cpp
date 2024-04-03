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
#include "Meta/ReflectedTypes/STD/ReflectSmartPtr.h"

void CE::AnimationRootComponent::SwitchAnimationRecursive(Registry& reg, const entt::entity entity, const std::shared_ptr<const Animation> animation, float timeStamp)
{
	auto skinnedMesh = reg.TryGet<SkinnedMeshComponent>(entity);

	if (skinnedMesh != nullptr)
	{
		skinnedMesh->mAnimation = animation;
		skinnedMesh->mCurrentTime = timeStamp;
	}

	auto transform = reg.TryGet<TransformComponent>(entity);

	if (transform == nullptr)
	{
		return;
	}

	for (const TransformComponent& child : transform->GetChildren())
	{
		SwitchAnimationRecursive(reg, child.GetOwner(), animation, timeStamp);
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

	SwitchAnimationRecursive(world->GetRegistry(), mOwner, mWantedAnimation, mWantedTimeStamp);
}

void CE::AnimationRootComponent::SwitchAnimation(Registry& reg, const std::shared_ptr<const Animation>& animation, float timeStamp)
{
	if (animation == mWantedAnimation)
	{
		return;
	}

	mWantedAnimation = animation;
	mWantedTimeStamp = timeStamp;

	SwitchAnimationRecursive(reg, mOwner, mWantedAnimation, mWantedTimeStamp);
}

CE::MetaType CE::AnimationRootComponent::Reflect()
{
	auto type = MetaType{MetaType::T<AnimationRootComponent>{}, "AnimationRootComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	type.AddField(&AnimationRootComponent::mWantedAnimation, "mWantedAnimation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&AnimationRootComponent::mWantedTimeStamp, "mWantedTimeStamp").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc(static_cast<void (AnimationRootComponent::*)()>(&AnimationRootComponent::SwitchAnimation), "SwitchAnimationEditor").GetProperties().Add(Props::sCallFromEditorTag);

	type.AddFunc([](AnimationRootComponent& animationRoot, const std::shared_ptr<const Animation>& animation, float timeStamp)
		{
			if (animation == nullptr)
			{
				LOG(LogWorld, Warning, "Attempted to set NULL animation.");
				return;
			}

			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			animationRoot.SwitchAnimation(world->GetRegistry(), animation, timeStamp);

		}, "SwitchAnimation", MetaFunc::ExplicitParams<AnimationRootComponent&,
		const std::shared_ptr<const Animation>&, float>{}, "AnimationRootComponent", "Animation", "Time Stamp").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	BindEvent(type, sConstructEvent, &AnimationRootComponent::OnConstruct);

	ReflectComponentType<AnimationRootComponent>(type);
	return type;
}
