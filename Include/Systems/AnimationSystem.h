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
			glm::vec3 mTranslation{};
			glm::vec3 mScale{};
			glm::quat mRotation{};
		};

		struct BakedAnimation
		{
			BakedAnimation(const AssetHandle<Animation>& animation, const AssetHandle<SkinnedMesh>& skinnedMesh);

			struct BakedFrame
			{
				std::array<AnimTransform, MAX_BONES> mBones{};
				uint32 mNumOfBonesInUse{};
			};
			std::vector<BakedFrame> mFrames{};
			static constexpr float sDesiredBakedFrameDuration = .2f;
			float mBakedFrameDuration{};
		};
		static void Blend(const BakedAnimation::BakedFrame& frame1, const BakedAnimation::BakedFrame& frame2, BakedAnimation::BakedFrame& output, float mixAmount);

		struct FramesToBlend
		{
			const BakedAnimation::BakedFrame& mFrame1;
			const BakedAnimation::BakedFrame& mFrame2;
			float mBlendWeight{};
		};

		std::optional<FramesToBlend> GetFramesToBlendBetween(
			const AssetHandle<Animation>& animation, 
			const AssetHandle<SkinnedMesh>& skinnedMesh,
			float currentDuration);

		std::unordered_map<size_t, BakedAnimation> mBakedAnimations{};

		std::vector<BoundEvent> mOnAnimationFinishEvents{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AnimationSystem);
	};
}
