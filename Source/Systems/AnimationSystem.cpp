#include "Precomp.h"
#include "Systems/AnimationSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Assets/SkinnedMesh.h"
#include "Components/AnimationRootComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/AttachToBoneComponent.h"
#include "Components/TransformComponent.h"
#include "Assets/Animation/Animation.h"
#include "Assets/Animation/Bone.h"
#include "Meta/MetaType.h"

CE::AnimationSystem::BakedAnimation::BakedAnimation(const AssetHandle<Animation>& animation,
	const AssetHandle<SkinnedMesh>& skinnedMesh)
{
	const auto& addBone = [&](const auto& self, const AnimNode& node, BakedFrame& frame, float time, const glm::mat4& parent = glm::mat4{1.0f})
		{
			const auto boneIt = skinnedMesh->GetBoneMap().find(node.mName);

			glm::mat4 nodeTransform = node.mTransform;

			if (boneIt != skinnedMesh->GetBoneMap().end())
			{
				const int index = boneIt->second.mId;

				if (index >= MAX_BONES)
				{
					LOG(LogAssets, Error, "Index {} is greater or equal to the the max number of bones ({})", index, MAX_BONES);
					return;
				}

				frame.mNumOfBonesInUse = glm::max(static_cast<uint32>(index + 1), frame.mNumOfBonesInUse);

				if (node.mBone != nullptr)
				{
					nodeTransform = node.mBone->GetInterpolatedTransform(time);
				}
			}

			const glm::mat4 globalTransform = parent * nodeTransform;

			if (boneIt != skinnedMesh->GetBoneMap().end())
			{
				const int index = boneIt->second.mId;
				const glm::mat4 finalTransform = globalTransform * boneIt->second.mOffset;
				const auto& [translation, scale, rotation] = TransformComponent::FromMatrix(finalTransform);

				frame.mBones[index] = { translation, scale, rotation };
			}

			for (const AnimNode& child : node.mChildren)
			{
				self(self, child, frame, time, globalTransform);
			}
		};

	const float durationInSeconds = animation->mDuration / animation->mTickPerSecond;
	const uint32 numOfFrames = static_cast<uint32>(durationInSeconds / (sDesiredBakedFrameDuration)) + 1u;

	mBakedFrameDuration = durationInSeconds / static_cast<float>(numOfFrames);

	mFrames.resize(numOfFrames + 1);

	for (uint32 i = 0; i <= numOfFrames; i++)
	{
		const float time = static_cast<float>(i) * mBakedFrameDuration * animation->mTickPerSecond;
		addBone(addBone, animation->mRootNode, mFrames[i], time);
	}
}

void CE::AnimationSystem::Blend(const BakedAnimation::BakedFrame& frame1, const BakedAnimation::BakedFrame& frame2,
	BakedAnimation::BakedFrame& output, float mixAmount)
{
	output.mNumOfBonesInUse = frame1.mNumOfBonesInUse;

	for (uint32 i = 0; i < frame1.mNumOfBonesInUse; i++)
	{
		const AnimTransform& T0 = frame1.mBones[i];
		const AnimTransform& T1 = frame2.mBones[i];

		AnimTransform transform{};
		transform.mTranslation = T0.mTranslation * mixAmount + T1.mTranslation * (1.0f - mixAmount);
		transform.mScale = T0.mScale * mixAmount + T1.mScale * (1.0f - mixAmount);
		transform.mRotation = glm::slerp(T1.mRotation, T0.mRotation, mixAmount);

		output.mBones[i] = transform;
	}
}

std::optional<CE::AnimationSystem::FramesToBlend> CE::AnimationSystem::GetFramesToBlendBetween(const AssetHandle<Animation>& animation, 
		const AssetHandle<SkinnedMesh>& skinnedMesh, 
		float timeStamp)
{
	uint32 hash = Internal::CombineHashes(
		Name::HashString(animation.GetMetaData().GetName()),
		Name::HashString(skinnedMesh.GetMetaData().GetName()));

	auto existingInfo = mBakedAnimations.find(hash);

	if (existingInfo == mBakedAnimations.end())
	{
		existingInfo = mBakedAnimations.emplace(std::piecewise_construct,
			std::forward_as_tuple(hash),
			std::forward_as_tuple(animation, skinnedMesh)).first;
	}

	const float timeStampInSeconds = timeStamp / animation->mTickPerSecond;
	const BakedAnimation& bakedAnimation = existingInfo->second;

	if (bakedAnimation.mFrames.empty())
	{
		LOG(LogRendering, Error, "Did not have enough frames");
		return std::nullopt;
	}

	const uint32 startIndex = static_cast<uint32>(timeStampInSeconds / bakedAnimation.mBakedFrameDuration);

	if (startIndex + 1 >= bakedAnimation.mFrames.size())
	{
		LOG(LogRendering, Error, "Timestamp {} was longer than the baked animation. Animation duration is {}", timeStamp, animation->mDuration);
		return std::nullopt;
	}

	const uint32 endIndex = startIndex + 1;

	const float firstFrameTimeStamp = static_cast<float>(startIndex) * bakedAnimation.mBakedFrameDuration;
	const float blendWeight = 1.0f - Math::lerpInv(firstFrameTimeStamp, firstFrameTimeStamp + bakedAnimation.mBakedFrameDuration, timeStampInSeconds);

	return FramesToBlend{ bakedAnimation.mFrames[startIndex], bakedAnimation.mFrames[endIndex], blendWeight };
}

CE::AnimationSystem::AnimationSystem() :
	mOnAnimationFinishEvents(GetAllBoundEvents(sOnAnimationFinish))
{
}

void CE::AnimationSystem::Update(World& world, float dt)
{
	Registry& reg = world.GetRegistry();

	for (auto [entity, animationRootComponent] : reg.View<AnimationRootComponent>().each())
	{
		if (animationRootComponent.mCurrentAnimation == nullptr)
		{
			continue;
		}

		animationRootComponent.mCurrentTimeStamp += animationRootComponent.mCurrentAnimation->mTickPerSecond * animationRootComponent.mCurrentAnimationSpeed * dt;

		if ((animationRootComponent.mCurrentTimeStamp / animationRootComponent.mCurrentAnimation->mDuration) > 1.0f)
		{
			animationRootComponent.mCurrentTimeStamp = fmod(animationRootComponent.mCurrentTimeStamp, animationRootComponent.mCurrentAnimation->mDuration);

			for (const BoundEvent& boundEvent : mOnAnimationFinishEvents)
			{
				entt::sparse_set* const storage = world.GetRegistry().Storage(boundEvent.mType.get().GetTypeId());

				if (storage == nullptr
					|| !storage->contains(entity))
				{
					continue;
				}

				if (boundEvent.mIsStatic)
				{
					boundEvent.mFunc.get().InvokeUncheckedUnpacked(world, entity);
				}
				else
				{
					MetaAny component{ boundEvent.mType, storage->value(entity), false };
					boundEvent.mFunc.get().InvokeUncheckedUnpacked(component, world, entity);
				}
			}
		}
	}

	static BakedAnimation::BakedFrame buffers[2]{};

	for (auto [entity, skinnedMesh] : reg.View<SkinnedMeshComponent>().each())
	{
		if (skinnedMesh.mSkinnedMesh == nullptr
			|| skinnedMesh.mAnimation == nullptr)
		{
			continue;
		}

		skinnedMesh.mCurrentTime += skinnedMesh.mAnimation->mTickPerSecond * skinnedMesh.mAnimationSpeed * dt;
		skinnedMesh.mCurrentTime = fmod(skinnedMesh.mCurrentTime, skinnedMesh.mAnimation->mDuration);

		const auto& framesToBlendBetween = GetFramesToBlendBetween(skinnedMesh.mAnimation, skinnedMesh.mSkinnedMesh, skinnedMesh.mCurrentTime);

		if (!framesToBlendBetween.has_value())
		{
			continue;
		}

		Blend(framesToBlendBetween->mFrame1, framesToBlendBetween->mFrame2, buffers[0], framesToBlendBetween->mBlendWeight);

		if (skinnedMesh.mPreviousAnimation != nullptr 
			&& skinnedMesh.mBlendWeight < 1.0f)
		{	
			skinnedMesh.mPrevAnimTime += skinnedMesh.mPreviousAnimation->mTickPerSecond * skinnedMesh.mAnimationSpeed * dt;
			skinnedMesh.mPrevAnimTime = fmod(skinnedMesh.mPrevAnimTime, skinnedMesh.mPreviousAnimation->mDuration);
			skinnedMesh.mBlendWeight = glm::clamp(skinnedMesh.mBlendWeight + 1.0f / skinnedMesh.mBlendTime * dt, 0.0f, 1.0f);

			const auto& prevFramesToBlendBetween = GetFramesToBlendBetween(skinnedMesh.mPreviousAnimation, skinnedMesh.mSkinnedMesh, skinnedMesh.mPrevAnimTime);

			if (prevFramesToBlendBetween.has_value())
			{
				// Mix the baked frames of the previous animation
				Blend(prevFramesToBlendBetween->mFrame1, prevFramesToBlendBetween->mFrame2, buffers[1], prevFramesToBlendBetween->mBlendWeight);

				// Mix the current animation and the previous animation
				Blend(buffers[0], buffers[1], buffers[0], skinnedMesh.mBlendWeight);
			}
		}

		for (uint32 i = 0; i < buffers[0].mNumOfBonesInUse; i++)
		{
			const AnimTransform& transform = buffers[0].mBones[i];
			skinnedMesh.mFinalBoneMatrices[i] = TransformComponent::ToMatrix(transform.mTranslation, transform.mScale, transform.mRotation);
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

		if (skinnedMesh == nullptr
			|| skinnedMesh->mSkinnedMesh == nullptr
			|| skinnedMesh->mAnimation == nullptr)
		{
			continue;
		}

		auto& boneMap = skinnedMesh->mSkinnedMesh->GetBoneMap();
		auto it = boneMap.find(attachToBone.mBoneName);

		if (it == boneMap.end())
		{
			transform.SetLocalMatrix(parent->GetWorldMatrix() * 
				TransformComponent::ToMatrix(attachToBone.mLocalTranslation, attachToBone.mLocalScale, attachToBone.mLocalRotation));
			continue;
		}

		const glm::mat4x4& boneMat = skinnedMesh->mFinalBoneMatrices[it->second.mId];
		transform.SetLocalMatrix(boneMat * TransformComponent::ToMatrix(attachToBone.mLocalTranslation, attachToBone.mLocalScale, attachToBone.mLocalRotation));
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
