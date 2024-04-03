#include "Precomp.h"
#include "Systems/AnimationSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/TransformComponent.h"
#include "Assets/SkinnedMesh.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/AnimationRootComponent.h"
#include "Assets/Animation/Animation.h"
#include "Assets/Animation/Bone.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

void CE::AnimationSystem::CalculateBoneTransformRecursive(const AnimNode& node, 
	const glm::mat4x4& parenTransform, 
	const std::unordered_map<std::string, BoneInfo>& boneMap,
	const SkinnedMeshComponent& mesh,
	const std::shared_ptr<const Animation> animation, 
	std::vector<glm::mat4x4>& finalBoneMatrices)
{
	const Bone* bone = animation->FindBone(node.mName);

	glm::mat4x4 nodeTransform = node.mTransform;
	
	if (bone)
	{
		nodeTransform = bone->GetInterpolatedTransform(mesh.mCurrentTime);
	}

	glm::mat4x4 globalTransform = parenTransform * nodeTransform;

	if (boneMap.find(node.mName) != boneMap.end())
	{
		int index = boneMap.at(node.mName).mId;
		
		finalBoneMatrices.at(index) = globalTransform * boneMap.at(node.mName).mOffset;
	}

	for (size_t i = 0; i < node.mChildren.size(); i++)
	{
		CalculateBoneTransformRecursive(node.mChildren[i], globalTransform, boneMap, mesh, mesh.mAnimation, finalBoneMatrices);
	}
}

void CE::AnimationSystem::SwitchAnimationRecursive(Registry& reg, const entt::entity entity, const std::shared_ptr<const Animation> animation, float timeStamp)
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

void CE::AnimationSystem::Update(World& world, float dt)
{
	auto& reg = world.GetRegistry();

	{
		const auto& view = reg.View<SkinnedMeshComponent>();

		for (auto [entity, skinnedMesh] : view.each())
		{
			if (skinnedMesh.mAnimation == nullptr)
			{
				continue;
			}

			skinnedMesh.mCurrentTime += skinnedMesh.mAnimation->mTickPerSecond * dt;
			skinnedMesh.mCurrentTime = fmod(skinnedMesh.mCurrentTime, skinnedMesh.mAnimation->mDuration);

			CalculateBoneTransformRecursive(skinnedMesh.mAnimation->mRootNode, glm::mat4x4(1.0f), skinnedMesh.mSkinnedMesh->GetBoneMap(), skinnedMesh, skinnedMesh.mAnimation, skinnedMesh.mFinalBoneMatrices);
		}
	}

	{
		const auto& view = reg.View<AnimationRootComponent>();

		for (auto [entity, animationRoot] : view.each())
		{
			if (animationRoot.mSwitchAnimation == false 
				|| animationRoot.mWantedAnimation == nullptr)
			{
				continue;
			}

			SwitchAnimationRecursive(reg, entity, animationRoot.mWantedAnimation, animationRoot.mWantedTimeStamp);
			animationRoot.mSwitchAnimation = false;
		}
	}
}

CE::MetaType CE::AnimationSystem::Reflect()
{
	return MetaType{ MetaType::T<AnimationSystem>{}, "AnimationSystem", MetaType::Base<System>{} };
}
