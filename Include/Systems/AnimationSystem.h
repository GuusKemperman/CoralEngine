#pragma once
#include "Assets/Animation/Animation.h"
#include "Assets/Core/AssetHandle.h"
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
		void Update(World& world, float dt) override;

	private:
		struct AnimMeshInfo
		{
			AnimMeshInfo(const AnimNode& node, const SkinnedMesh& mesh);
			std::reference_wrapper<const AnimNode> mAnimNode;
			const BoneInfo* mBoneInfo{};
			std::vector<AnimMeshInfo> mChildren{};
		};

		struct Transform
		{
			glm::vec3 mTranslation;
			glm::vec3 mScale;
			glm::quat mRotation;
		};

		void CalculateBoneTransformRecursive(const AnimMeshInfo& animMeshInfo,
			const glm::mat4x4& parenTransform,
			SkinnedMeshComponent& meshComponent);

		void BlendAnimations(SkinnedMeshComponent& meshComponent);
		void CalculateTransformsRecursive(const AnimMeshInfo& animMeshInfo,	SkinnedMeshComponent& meshComponent, std::vector<Transform>& output);
		void BlendTransformsRecursive(const AnimMeshInfo& animMeshInfo, const glm::mat4x4& parenTransform, SkinnedMeshComponent& meshComponent, const std::vector<Transform>& layer0, const std::vector<Transform>& layer1);

		std::unordered_map<size_t, AnimMeshInfo> mAnimMeshInfoMap{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AnimationSystem);
	};
}
