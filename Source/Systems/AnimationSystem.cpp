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

CE::AnimationSystem::AnimMeshInfo& CE::AnimationSystem::FindAnimMeshInfo(const AssetHandle<Animation> animation, const AssetHandle<SkinnedMesh> skinnedMesh)
{
	uint32 hash = Internal::CombineHashes(
	Name::HashString(animation.GetMetaData().GetName()),
	Name::HashString(skinnedMesh.GetMetaData().GetName()));
		
	auto existingInfo = mAnimMeshInfoMap.find(hash);

	if (existingInfo == mAnimMeshInfoMap.end())
	{
		existingInfo = mAnimMeshInfoMap.emplace(std::piecewise_construct,
			std::forward_as_tuple(hash),
			std::forward_as_tuple(animation->mRootNode, *skinnedMesh)).first;
	}

	return existingInfo->second;
}

void CE::AnimationSystem::CalculateBoneTransformsRecursive(const AnimMeshInfo& animMeshInfo,
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
		CalculateBoneTransformsRecursive(child, globalTransform, meshComponent);
	}
}

void CE::AnimationSystem::BlendAnimations(SkinnedMeshComponent& meshComponent)
{
	static std::array<AnimTransform, MAX_BONES> layer0;
	static std::array<AnimTransform, MAX_BONES> layer1;

	const AnimMeshInfo& info0 = FindAnimMeshInfo(meshComponent.mAnimation, meshComponent.mSkinnedMesh);
	const AnimMeshInfo& info1 = FindAnimMeshInfo(meshComponent.mPreviousAnimation, meshComponent.mSkinnedMesh);

	CalculateAnimTransformsRecursive(info0, meshComponent, meshComponent.mCurrentTime, layer0);
	CalculateAnimTransformsRecursive(info1, meshComponent, meshComponent.mPrevAnimTime, layer1);

	BlendAnimTransformsRecursive(info0, glm::mat4x4{ 1.0f }, meshComponent, layer0, layer1);
}

void CE::AnimationSystem::CalculateAnimTransformsRecursive(const AnimMeshInfo& animMeshInfo, SkinnedMeshComponent& meshComponent, float timeStamp, std::array<AnimTransform, MAX_BONES>& output)
{
	const Bone* bone = animMeshInfo.mAnimNode.get().mBone;

	if (animMeshInfo.mBoneInfo != nullptr
		&& bone != nullptr)
	{
		const int index = animMeshInfo.mBoneInfo->mId;
		
		AnimTransform transform{};

		transform.mTranslation = bone->InterpolatePosition(timeStamp);
		transform.mScale = bone->InterpolateScale(timeStamp);
		transform.mRotation = bone->InterpolateRotation(timeStamp);

		output[index] = transform;
	}

	for (const AnimMeshInfo& child : animMeshInfo.mChildren)
	{
		CalculateAnimTransformsRecursive(child, meshComponent, timeStamp, output);
	}
}

void CE::AnimationSystem::BlendAnimTransformsRecursive(const AnimMeshInfo& animMeshInfo, const glm::mat4x4& parenTransform, SkinnedMeshComponent& meshComponent, const std::array<AnimTransform, MAX_BONES>& layer0, const std::array<AnimTransform, MAX_BONES>& layer1)
{
	glm::mat4x4 globalTransform = animMeshInfo.mAnimNode.get().mTransform;

	if (animMeshInfo.mBoneInfo != nullptr)
	{
		const int index = animMeshInfo.mBoneInfo->mId;

		const AnimTransform& T0 = layer0[index];
		const AnimTransform& T1 = layer1[index];

		AnimTransform transform{};

		transform.mTranslation = T0.mTranslation * meshComponent.mBlendWeight + T1.mTranslation * (1.0f - meshComponent.mBlendWeight);
		transform.mScale	   = T0.mScale * meshComponent.mBlendWeight + T1.mScale * (1.0f - meshComponent.mBlendWeight);
		transform.mRotation	   = glm::slerp(T1.mRotation, T0.mRotation, meshComponent.mBlendWeight);

		globalTransform = parenTransform * TransformComponent::ToMatrix(transform.mTranslation, transform.mScale, transform.mRotation);

		meshComponent.mFinalBoneMatrices[index] = globalTransform * animMeshInfo.mBoneInfo->mOffset;
	}

	for (const AnimMeshInfo& child : animMeshInfo.mChildren)
	{
		BlendAnimTransformsRecursive(child, globalTransform, meshComponent, layer0, layer1);
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

		if (skinnedMesh.mPreviousAnimation != nullptr 
			&& skinnedMesh.mBlendWeight < 1.0f)
		{	
			skinnedMesh.mPrevAnimTime += skinnedMesh.mPreviousAnimation->mTickPerSecond * skinnedMesh.mAnimationSpeed * dt;
			skinnedMesh.mPrevAnimTime = fmod(skinnedMesh.mPrevAnimTime, skinnedMesh.mPreviousAnimation->mDuration);
			skinnedMesh.mBlendWeight = glm::clamp(skinnedMesh.mBlendWeight + 1.0f / skinnedMesh.mBlendTime * dt, 0.0f, 1.0f);

			BlendAnimations(skinnedMesh);
		}
		else // Only a single animation is being played
		{
			const AnimMeshInfo& info = FindAnimMeshInfo(skinnedMesh.mAnimation, skinnedMesh.mSkinnedMesh);

			CalculateBoneTransformsRecursive(info, glm::mat4x4{ 1.0f }, skinnedMesh);
		}
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

		if (skinnedMesh == nullptr
			|| skinnedMesh->mSkinnedMesh == nullptr)
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
