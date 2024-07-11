#include "Precomp.h"
#include "Components/SkinnedMeshComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/SkinnedMesh.h"
#include "Assets/Material.h"
#include "Assets/Animation/Animation.h"

CE::MetaType CE::SkinnedMeshComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<SkinnedMeshComponent>{}, "SkinnedMeshComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&SkinnedMeshComponent::mSkinnedMesh, "mSkinnnedMesh").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&SkinnedMeshComponent::mMaterial, "mMaterial").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&SkinnedMeshComponent::mAnimation, "mAnimation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&SkinnedMeshComponent::mPreviousAnimation, "mPreviousAnimation").GetProperties().Add(Props::sIsEditorReadOnlyTag);
	type.AddField(&SkinnedMeshComponent::mCurrentTime, "mCurrentTime").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&SkinnedMeshComponent::mPrevAnimTime, "mPrevAnimTime").GetProperties().Add(Props::sIsEditorReadOnlyTag);
	type.AddField(&SkinnedMeshComponent::mAnimationSpeed, "mAnimationSpeed").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&SkinnedMeshComponent::mBlendTime, "mBlendTime").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&SkinnedMeshComponent::mBlendWeight, "mBlendWeight").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&SkinnedMeshComponent::mHighlightedMesh, "mHighlightedMesh").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<SkinnedMeshComponent>(type);
	return type;
}