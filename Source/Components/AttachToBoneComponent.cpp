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
void CE::AttachToBoneComponent::OnInspect(World& world, const entt::entity owner)
{
	AssetHandle<SkinnedMesh> skinnedMesh{};
	std::string bone{};

	Registry& reg = world.GetRegistry();

	const TransformComponent* transform = reg.TryGet<TransformComponent>(owner);

	if (transform == nullptr
		|| transform->GetParent() == nullptr)
	{
		return;
	}

	const SkinnedMeshComponent* skinnedMeshComponent = FindSkinnedMeshParentRecursive(reg, *transform->GetParent());

	if (skinnedMeshComponent == nullptr)
	{
		return;
	}

	if (skinnedMesh != nullptr && skinnedMesh != skinnedMeshComponent->mSkinnedMesh)
	{
		ImGui::TextWrapped("Selected entities have different skinned meshes");
		return;
	}

	AttachToBoneComponent& boneComponent = reg.Get<AttachToBoneComponent>(owner);

	skinnedMesh = skinnedMeshComponent->mSkinnedMesh;
	bone = boneComponent.mBoneName;

	if (bone.empty())
	{
		bone = "None";
	}

	if (skinnedMesh == nullptr)
	{
		ImGui::Text("No SkinnedMesh in hierarchy above");
		return;
	}

	if (Search::BeginCombo("mBone", bone))
	{
		if (Search::Button("None"))
		{
			bone.clear();
		}

		for (const auto [boneName, _] : skinnedMesh->GetBoneMap())
		{
			if (Search::Button(boneName))
			{
				bone = boneName;
			}
		}

		boneComponent.mBoneName = bone;
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
	BindEvent(type, sOnInspect, &AttachToBoneComponent::OnInspect);
#endif // EDITOR
	BindEvent(type, sOnConstruct, &AttachToBoneComponent::OnConstruct);

	ReflectComponentType<AttachToBoneComponent>(type);
	return type;
}
