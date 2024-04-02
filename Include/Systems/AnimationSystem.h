#pragma once
#include "Systems/System.h"

namespace CE
{
	class Registry;
	class Animation;
	class SkinnedMeshComponent;
	struct AnimNode;
	struct BoneInfo;

	class AnimationSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		void CalculateBoneTransformRecursive(const AnimNode& node, 
	const glm::mat4x4& parenTransform, 
	const std::unordered_map<std::string, BoneInfo>& boneMap,
	const SkinnedMeshComponent& mesh,
	const std::shared_ptr<const Animation> animation, 
	std::vector<glm::mat4x4>& finalBoneMatrices);

		void SwitchAnimationRecursive(Registry& reg, const entt::entity entity, const std::shared_ptr<const Animation> animation, float timeStamp);

	private:

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AnimationSystem);
	};
}