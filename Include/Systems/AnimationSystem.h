#pragma once
#include "Systems/System.h"
#include <unordered_map>

namespace Engine
{
	class Animation;
	class SkinnedMesh;
	struct AnimNode;
	struct BoneInfo;

	class AnimationSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;
		//void Render(const World& world) override;

		void CalculateBoneTransform(const Engine::AnimNode& node, const glm::mat4& parenTransform);

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits {};
			traits.mShouldTickBeforeBeginPlay = false;
			traits.mShouldTickWhilstPaused = false;
			return traits;
		}

	private:

		std::shared_ptr<const Animation> mCurrentAnimation;
		float mCurrentAnimationTime;
		const std::unordered_map<std::string, BoneInfo>* mCurrentBoneMap; 
		std::vector<glm::mat4x4>* mCurrentFinalBoneMatrices;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AnimationSystem);
	};
}