#include "Precomp.h"
#include "Systems/AnimationSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Assets/SkinnedMesh.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/AttachToBoneComponent.h"
#include "Components/TransformComponent.h"
#include "Assets/Animation/Animation.h"
#include "Assets/Animation/Bone.h"
#include "Meta/MetaType.h"


void CE::AnimationSystem::CalculateBoneTransformRecursive(const AnimMeshInfo& animMeshInfo,
	const glm::mat4x4& parenTransform, 
	SkinnedMeshComponent& meshComponent)
{
	glm::mat4x4 nodeTransform = animMeshInfo.mAnimNode.get().mTransform;
	
	if (animMeshInfo.mAnimNode.get().mBone != nullptr)
	{
		nodeTransform = animMeshInfo.mAnimNode.get().mBone->GetInterpolatedTransform(meshComponent.mCurrentTime);
	}

	const glm::mat4x4 globalTransform = parenTransform * nodeTransform;

	if (animMeshInfo.mBoneInfo != nullptr)
	{
		const int index = animMeshInfo.mBoneInfo->mId;
		meshComponent.mFinalBoneMatrices[index] = globalTransform * animMeshInfo.mBoneInfo->mOffset;
	}

	for (const AnimMeshInfo& child : animMeshInfo.mChildren)
	{
		CalculateBoneTransformRecursive(child, globalTransform, meshComponent);
	}
}

void CE::AnimationSystem::Update(World& world, float dt)
{
	Registry& reg = world.GetRegistry();

	for (auto [entity, skinnedMesh] : reg.View<SkinnedMeshComponent>().each())
	{
		if (skinnedMesh.mSkinnedMesh == nullptr)
		{
			continue;
		}

		if (skinnedMesh.mFinalBoneMatrices.size() != skinnedMesh.mSkinnedMesh->GetBoneMap().size())
		{
			size_t newSize = glm::min(static_cast<size_t>(MAX_BONES), skinnedMesh.mSkinnedMesh->GetBoneMap().size());
			skinnedMesh.mFinalBoneMatrices = std::vector<glm::mat4x4>(newSize, glm::mat4x4(1.0f));
		}
		
		if (skinnedMesh.mAnimation == nullptr)
		{
			continue;
		}

		skinnedMesh.mCurrentTime += skinnedMesh.mAnimation->mTickPerSecond * skinnedMesh.mAnimationSpeed * dt;
		skinnedMesh.mCurrentTime = fmod(skinnedMesh.mCurrentTime, skinnedMesh.mAnimation->mDuration);

		const uint32 hash = Internal::CombineHashes(
			Name::HashString(skinnedMesh.mAnimation.GetMetaData().GetName()),
			Name::HashString(skinnedMesh.mSkinnedMesh.GetMetaData().GetName()));
		
		auto existingInfo = mAnimMeshInfoMap.find(hash);

		if (existingInfo == mAnimMeshInfoMap.end())
		{
			existingInfo = mAnimMeshInfoMap.emplace(std::piecewise_construct,
				std::forward_as_tuple(hash),
				std::forward_as_tuple(skinnedMesh.mAnimation->mRootNode, *skinnedMesh.mSkinnedMesh)).first;
		}

		CalculateBoneTransformRecursive(existingInfo->second, glm::mat4x4{ 1.0f }, skinnedMesh);
	}

	for (auto [entity, attachToBone, transform] : reg.View<AttachToBoneComponent, TransformComponent>().each())
	{
		if (attachToBone.mBoneName.empty())
		{
			continue;
		}

		const TransformComponent* parent = transform.GetParent();

		if (parent == nullptr)
		{
			continue;
		}

		const SkinnedMeshComponent* skinnedMesh = AttachToBoneComponent::FindSkinnedMeshParentRecursive(reg, *parent);

		if (skinnedMesh == nullptr)
		{
			continue;
		}

		auto& boneMap = skinnedMesh->mSkinnedMesh->GetBoneMap();
		auto it = boneMap.find(attachToBone.mBoneName);

		if (it == boneMap.end() 
			|| skinnedMesh->mAnimation == nullptr)
		{
			transform.SetLocalMatrix(parent->GetWorldMatrix() * 
				TransformComponent::ToMatrix(attachToBone.mLocalTranslation, attachToBone.mLocalScale, attachToBone.mLocalRotation));
			continue;
		}

		const glm::mat4x4& boneMat = skinnedMesh->mFinalBoneMatrices[it->second.mId];
	
		transform.SetLocalMatrix(boneMat *
			TransformComponent::ToMatrix(attachToBone.mLocalTranslation, attachToBone.mLocalScale, attachToBone.mLocalRotation));
	}
}

CE::AnimationSystem::AnimMeshInfo::AnimMeshInfo(const AnimNode& node, const SkinnedMesh& mesh) :
	mAnimNode(node)
{
	const auto boneIt = mesh.GetBoneMap().find(node.mName);

	if (boneIt != mesh.GetBoneMap().end())
	{
		mBoneInfo = &boneIt->second;
	}

	mChildren.reserve(node.mChildren.size());
	for (const AnimNode& child : node.mChildren)
	{
		mChildren.emplace_back(child, mesh);
	}
}

CE::MetaType CE::AnimationSystem::Reflect()
{
	return MetaType{ MetaType::T<AnimationSystem>{}, "AnimationSystem", MetaType::Base<System>{} };
}
