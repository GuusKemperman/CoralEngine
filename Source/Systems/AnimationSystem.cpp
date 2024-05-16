#include "Precomp.h"
#include "Systems/AnimationSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Assets/SkinnedMesh.h"
#include "Components/SkinnedMeshComponent.h"
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

void CE::AnimationSystem::BlendAnimations(SkinnedMeshComponent& meshComponent)
{
	static std::vector<Transform> layer0{MAX_BONES};
	static std::vector<Transform> layer1{MAX_BONES};

	// Layer 0
	uint32 hash = Internal::CombineHashes(
	Name::HashString(meshComponent.mAnimation.GetMetaData().GetName()),
	Name::HashString(meshComponent.mSkinnedMesh.GetMetaData().GetName()));
		
	auto existingInfo = mAnimMeshInfoMap.find(hash);

	if (existingInfo == mAnimMeshInfoMap.end())
	{
		existingInfo = mAnimMeshInfoMap.emplace(std::piecewise_construct,
			std::forward_as_tuple(hash),
			std::forward_as_tuple(meshComponent.mAnimation->mRootNode, *meshComponent.mSkinnedMesh)).first;
	}

	CalculateTransformsRecursive(existingInfo->second, meshComponent, meshComponent.mCurrentTime, layer0);

	// Layer 1

	hash = Internal::CombineHashes(
	Name::HashString(meshComponent.mPreviousAnimation.GetMetaData().GetName()),
	Name::HashString(meshComponent.mSkinnedMesh.GetMetaData().GetName()));

	auto existingInfo2 = mAnimMeshInfoMap.find(hash);

	if (existingInfo2 == mAnimMeshInfoMap.end())
	{
		existingInfo2 = mAnimMeshInfoMap.emplace(std::piecewise_construct,
			std::forward_as_tuple(hash),
			std::forward_as_tuple(meshComponent.mPreviousAnimation->mRootNode, *meshComponent.mSkinnedMesh)).first;
	}

	CalculateTransformsRecursive(existingInfo2->second, meshComponent, meshComponent.mPrevAnimTime, layer1);

	// Blending

	BlendTransformsRecursive(existingInfo->second, glm::mat4x4{ 1.0f }, meshComponent, layer0, layer1);

}

void CE::AnimationSystem::CalculateTransformsRecursive(const AnimMeshInfo& animMeshInfo, SkinnedMeshComponent& meshComponent, float timeStamp, std::vector<Transform>& output)
{
	const Bone* bone = animMeshInfo.mAnimNode.get().mBone;

	if (animMeshInfo.mBoneInfo != nullptr
		&& bone != nullptr)
	{
		const int index = animMeshInfo.mBoneInfo->mId;
		
		Transform transform{};

		transform.mTranslation = bone->InterpolatePosition(timeStamp);
		transform.mScale = bone->InterpolateScale(timeStamp);
		transform.mRotation = bone->InterpolateRotation(timeStamp);

		output[index] = transform;
	}

	for (const AnimMeshInfo& child : animMeshInfo.mChildren)
	{
		CalculateTransformsRecursive(child, meshComponent, timeStamp, output);
	}
}

void CE::AnimationSystem::BlendTransformsRecursive(const AnimMeshInfo& animMeshInfo, const glm::mat4x4& parenTransform, SkinnedMeshComponent& meshComponent, const std::vector<Transform>& layer0, const std::vector<Transform>& layer1)
{
	glm::mat4x4 globalTransform = animMeshInfo.mAnimNode.get().mTransform;

	if (animMeshInfo.mBoneInfo != nullptr)
	{
		const int index = animMeshInfo.mBoneInfo->mId;

		const Transform& T0 = layer0[index];
		const Transform& T1 = layer1[index];

		Transform transform{};

		transform.mTranslation = T0.mTranslation * meshComponent.mBlendWeight + T1.mTranslation * (1.0f - meshComponent.mBlendWeight);
		transform.mScale	   = T0.mScale * meshComponent.mBlendWeight + T1.mScale * (1.0f - meshComponent.mBlendWeight);
		transform.mRotation	   = glm::slerp(T1.mRotation, T0.mRotation, meshComponent.mBlendWeight);

		globalTransform = parenTransform * TransformComponent::ToMatrix(transform.mTranslation, transform.mScale, transform.mRotation);

		meshComponent.mFinalBoneMatrices[index] = globalTransform * animMeshInfo.mBoneInfo->mOffset;
	}

	for (const AnimMeshInfo& child : animMeshInfo.mChildren)
	{
		BlendTransformsRecursive(child, globalTransform, meshComponent, layer0, layer1);
	}
}

void CE::AnimationSystem::Update(World& world, float dt)
{
	Registry& reg = world.GetRegistry();

	for (auto [entity, skinnedMesh] : reg.View<SkinnedMeshComponent>().each())
	{
		if (skinnedMesh.mFinalBoneMatrices.size() != skinnedMesh.mSkinnedMesh->GetBoneMap().size())
		{
			size_t newSize = glm::min(static_cast<size_t>(MAX_BONES), skinnedMesh.mSkinnedMesh->GetBoneMap().size());
			skinnedMesh.mFinalBoneMatrices = std::vector<glm::mat4x4>(newSize, glm::mat4x4(1.0f));
		}
		
		if (skinnedMesh.mAnimation == nullptr
			|| skinnedMesh.mSkinnedMesh == nullptr)
		{
			continue;
		}

		skinnedMesh.mCurrentTime += skinnedMesh.mAnimation->mTickPerSecond * skinnedMesh.mAnimationSpeed * dt;
		skinnedMesh.mCurrentTime = fmod(skinnedMesh.mCurrentTime, skinnedMesh.mAnimation->mDuration);

		if (skinnedMesh.mPreviousAnimation != nullptr)
		{
			skinnedMesh.mPrevAnimTime += skinnedMesh.mPreviousAnimation->mTickPerSecond * skinnedMesh.mAnimationSpeed * dt;
			skinnedMesh.mPrevAnimTime = fmod(skinnedMesh.mPrevAnimTime, skinnedMesh.mPreviousAnimation->mDuration);
			skinnedMesh.mBlendWeight = glm::clamp(skinnedMesh.mBlendWeight + 1.0f / skinnedMesh.mBlendSpeed * dt, 0.0f, 1.0f);
		}

		if (skinnedMesh.mBlendWeight < 1.0f)
		{	// Need to do blending between 2 animations
			BlendAnimations(skinnedMesh);
		}
		else // Only a single animation is being played
		{
			skinnedMesh.mPreviousAnimation = nullptr;

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
