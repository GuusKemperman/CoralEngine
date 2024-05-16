#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;
	class Registry;
	class SkinnedMeshComponent;
	class TransformComponent;

	class AttachToBoneComponent
	{
	public:
		void OnConstruct(World&, entt::entity owner);
		static SkinnedMeshComponent* FindSkinnedMeshParentRecursive(Registry& reg, const TransformComponent& transform);

		entt::entity mOwner{};

		std::string mBoneName{};

		glm::vec3 mLocalTranslation{0.0f};
		glm::vec3 mLocalScale{1.0f};
		glm::quat mLocalRotation = glm::identity<glm::quat>();

	private:
#ifdef EDITOR
		static void OnInspect(World& world, const std::vector<entt::entity>& entities);
#endif // EDITOR

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AttachToBoneComponent);
	};
}