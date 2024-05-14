#include "Precomp.h"
#include "Components/AttachToBoneComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::AttachToBoneComponent::OnConstruct(World&, entt::entity owner)
{
	mOwner = owner;
}

CE::MetaType CE::AttachToBoneComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<AttachToBoneComponent>{}, "AttachToBoneComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&AttachToBoneComponent::mLocalTranslation, "mLocalTranslation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&AttachToBoneComponent::mLocalRotation, "mLocalRotation").GetProperties().Add(Props::sIsScriptableTag);

	// Todo: field to select bone in parent skinned mesh

	ReflectComponentType<AttachToBoneComponent>(type);
	return type;
}
