#include "Precomp.h"
#include "Systems/AnimationSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "World/WorldRenderer.h"
#include "Components/TransformComponent.h"
#include "Assets/SkinnedMesh.h"
#include "Components/SkinnedMeshComponent.h"
#include "Assets/Animation.h"
#include "Components/Animation/Bone.h"
#include "Utilities/DebugRenderer.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

void Engine::AnimationSystem::CalculateBoneTransform(const AnimNode& node, const glm::mat4& parenTransform)
{
	const Bone* bone = mCurrentAnimation->FindBone(node.mName);

	glm::mat4x4 nodeTransform = node.mTransform;
	
	if (bone)
	{
		nodeTransform = bone->GetInterpolatedTransform(mCurrentAnimationTime);
	}

	glm::mat4x4 globalTransform = parenTransform * nodeTransform;

	if (mCurrentBoneMap->find(node.mName) != mCurrentBoneMap->end())
	{
		int index = mCurrentBoneMap->at(node.mName).mId;
		
		mCurrentFinalBoneMatrices->at(index) = globalTransform * mCurrentBoneMap->at(node.mName).mOffset;
	}

	for (size_t i = 0; i < node.mChildren.size(); i++)
	{
		CalculateBoneTransform(node.mChildren[i], globalTransform);
	}
}

void Engine::AnimationSystem::Update(World& world, float dt)
{
	auto& reg = world.GetRegistry();

	const auto& view = reg.View<SkinnedMeshComponent>();

	for (auto [entity, skinnedMesh] : view.each())
	{
		if (skinnedMesh.mAnimation == nullptr)
		{
			continue;
		}

		mCurrentAnimation = skinnedMesh.mAnimation;
		mCurrentFinalBoneMatrices = &skinnedMesh.mFinalBoneMatrices;

		skinnedMesh.mCurrentTime += skinnedMesh.mAnimation->mTickPerSecond * dt;
		skinnedMesh.mCurrentTime = fmod(skinnedMesh.mCurrentTime, skinnedMesh.mAnimation->mDuration);

		mCurrentAnimationTime = skinnedMesh.mCurrentTime;
		mCurrentBoneMap = skinnedMesh.mSkinnedMesh->GetBoneMap();

		CalculateBoneTransform(skinnedMesh.mAnimation->mRootNode, glm::mat4x4(1.0f));
	}

	mCurrentAnimation = nullptr;
	mCurrentAnimationTime = 0.0f;
	mCurrentBoneMap = nullptr;
	mCurrentFinalBoneMatrices = nullptr;
}

//void Engine::AnimationSystem::Render(const World& world)
//{
//	
//	// Render bones with debug lines
//}

Engine::MetaType Engine::AnimationSystem::Reflect()
{
	return MetaType{ MetaType::T<AnimationSystem>{}, "AnimationSystem", MetaType::Base<System>{} };
}
