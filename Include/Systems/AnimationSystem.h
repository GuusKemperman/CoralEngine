#pragma once
#include "Assets/Animation/Animation.h"
#include "Assets/Core/AssetHandle.h"
#include "Rendering/SkinnedMeshDefines.h"
#include "Utilities/Events.h"
#include "Systems/System.h"

namespace CE
{
	class Registry;
	class Animation;
	class SkinnedMesh;
	class SkinnedMeshComponent;
	struct AnimNode;
	struct BoneInfo;

	class AnimationSystem final :
		public System
	{
	public:
		AnimationSystem();

		void Update(World& world, float dt) override;

	private:
		struct AnimMeshInfo
		{
			AnimMeshInfo(const AnimNode& node, const SkinnedMesh& mesh);
			std::reference_wrapper<const AnimNode> mAnimNode;
			const BoneInfo* mBoneInfo{};
			std::vector<AnimMeshInfo> mChildren{};
		};

		struct AnimTransform
		{
			glm::vec3 mTranslation;
			glm::vec3 mScale;
			glm::quat mRotation;
		};

		AnimMeshInfo& FindAnimMeshInfo(const AssetHandle<Animation> animation, const AssetHandle<SkinnedMesh> skinnedMesh);

		void CalculateBoneTransformsRecursive(const AnimMeshInfo& animMeshInfo, const glm::mat4x4& parenTransform, SkinnedMeshComponent& meshComponent);

		void BlendAnimations(SkinnedMeshComponent& meshComponent);
		void CalculateAnimTransformsRecursive(const AnimMeshInfo& animMeshInfo,	SkinnedMeshComponent& meshComponent, float timeStamp, std::array<AnimTransform, MAX_BONES>& output);
		void BlendAnimTransformsRecursive(const AnimMeshInfo& animMeshInfo, const glm::mat4x4& parenTransform, SkinnedMeshComponent& meshComponent, const std::array<AnimTransform, MAX_BONES>& layer0, const std::array<AnimTransform, MAX_BONES>& layer1);

		std::unordered_map<size_t, AnimMeshInfo> mAnimMeshInfoMap{};

		std::vector<BoundEvent> mOnAnimationFinishEvents{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AnimationSystem);
	};
}
