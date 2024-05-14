#include "Precomp.h"
#include "Components/AttachToBoneComponent.h"

#include "Components/SkinnedMeshComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Assets/SkinnedMesh.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::AttachToBoneComponent::OnInspect(World& world, const std::vector<entt::entity>& entities)
{
	AssetHandle<SkinnedMesh> skinnedMesh{};
	std::string bone{};
	bool areAllBonesTheSame = true;

	Registry& reg = world.GetRegistry();

	for (entt::entity entity : entities)
	{
		const SkinnedMeshComponent* skinnedMeshComponent = reg.TryGet<SkinnedMeshComponent>(entity);

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

CE::MetaType CE::AttachToBoneComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<AttachToBoneComponent>{}, "AttachToBoneComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&AttachToBoneComponent::mBoneName, "mBone").GetProperties().Add(Props::sNoInspectTag);



	type.AddField(&AttachToBoneComponent::mLocalTranslation, "mLocalTranslation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&AttachToBoneComponent::mLocalRotation, "mLocalRotation").GetProperties().Add(Props::sIsScriptableTag);

	// Todo: field to select bone in parent skinned mesh
	BindEvent(type, sInspectEvent, &AttachToBoneComponent::OnInspect);

	ReflectComponentType<AttachToBoneComponent>(type);
	return type;
}
