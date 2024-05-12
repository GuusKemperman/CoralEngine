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

		void CalculateBoneTransformRecursive(const AnimMeshInfo& animMeshInfo,
			const glm::mat4x4& parenTransform,
			SkinnedMeshComponent& meshComponent);

		std::unordered_map<size_t, AnimMeshInfo> mAnimMeshInfoMap{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AnimationSystem);
	};
}
