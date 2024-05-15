#include "Precomp.h"
#include "Components/AttachToBoneComponent.h"

#include "Components/SkinnedMeshComponent.h"
#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Assets/SkinnedMesh.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/World.h"

void CE::AttachToBoneComponent::OnConstruct(World&, entt::entity owner)
{
	mOwner = owner;
}

#ifdef EDITOR
void CE::AttachToBoneComponent::OnInspect(World& world, const std::vector<entt::entity>& entities)
{
	AssetHandle<SkinnedMesh> skinnedMesh{};
	std::string bone{};
	bool areAllBonesTheSame = true;

	Registry& reg = world.GetRegistry();

	for (entt::entity entity : entities)
	{
		const TransformComponent* transform = reg.TryGet<TransformComponent>(entity);

		if (transform == nullptr)
		{
			continue;
		}

		const SkinnedMeshComponent* skinnedMeshComponent = FindSkinnedMeshParentRecursive(reg, *transform);

		if (skinnedMeshComponent == nullptr)
		{
			continue;
		}

		if (skinnedMesh != nullptr && skinnedMesh != skinnedMeshComponent->mSkinnedMesh)
		{
			ImGui::TextWrapped("Selected entities have different skinned meshes");
			return;
		}

		const AttachToBoneComponent& boneComponent = reg.Get<AttachToBoneComponent>(entity);

		skinnedMesh = skinnedMeshComponent->mSkinnedMesh;

		if (entities[0] != entity
			&& bone != boneComponent.mBoneName)
		{
			areAllBonesTheSame = false;
		}
		bone = boneComponent.mBoneName;
	}

	std::string_view bonePreviewName = areAllBonesTheSame ? std::string_view{ bone } : std::string_view{ "* Different bones" };

	if (bonePreviewName.empty())
	{
		bonePreviewName = "None";
	}

	if (Search::BeginCombo("mBone", bonePreviewName))
	{
		if (Search::Button("None"))
		{
			bone.clear();
		}

		if (skinnedMesh == nullptr)
		{
			return;
		}

		for (const auto [boneName, _] : skinnedMesh->GetBoneMap())
		{
			if (Search::Button(boneName))
			{
				bone = boneName;
			}
		}

		for (entt::entity entity : entities)
		{
			AttachToBoneComponent& boneComponent = reg.Get<AttachToBoneComponent>(entity);
			boneComponent.mBoneName = bone;
		}

		Search::EndCombo();
	}
}
#endif // EDITOR

CE::SkinnedMeshComponent* CE::AttachToBoneComponent::FindSkinnedMeshParentRecursive(Registry& reg, const TransformComponent& transform)
{
	SkinnedMeshComponent* skinnedMesh = reg.TryGet<SkinnedMeshComponent>(transform.GetOwner()); 

	if (skinnedMesh != nullptr)
	{
		return skinnedMesh;
	}

	const TransformComponent* parent = transform.GetParent();

	if (parent == nullptr)
	{
		return nullptr;
	}

	return FindSkinnedMeshParentRecursive(reg, *parent);
}


CE::MetaType CE::AttachToBoneComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<AttachToBoneComponent>{}, "AttachToBoneComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&AttachToBoneComponent::mBoneName, "mBone").GetProperties().Add(Props::sNoInspectTag);
	type.AddField(&AttachToBoneComponent::mLocalTranslation, "mLocalTranslation");
	type.AddField(&AttachToBoneComponent::mLocalRotation, "mLocalRotation");
	type.AddField(&AttachToBoneComponent::mLocalScale, "mLocalScale");

	type.AddFunc([](AttachToBoneComponent& attachToBone)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			TransformComponent* transform = world->GetRegistry().TryGet<TransformComponent>(attachToBone.mOwner);

			attachToBone.mLocalTranslation = transform->GetLocalPosition();
			attachToBone.mLocalRotation = transform->GetLocalOrientation();
			attachToBone.mLocalScale = transform->GetLocalScale();

		}, "CopyTransformValues", MetaFunc::ExplicitParams<AttachToBoneComponent&>{}
		).GetProperties().Add(Props::sCallFromEditorTag);
#ifdef EDITOR
	BindEvent(type, sInspectEvent, &AttachToBoneComponent::OnInspect);
#endif // EDITOR
	BindEvent(type, sConstructEvent, &AttachToBoneComponent::OnConstruct);

	ReflectComponentType<AttachToBoneComponent>(type);
	return type;
}
