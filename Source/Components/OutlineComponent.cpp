#include "Precomp.h"
#include "Components/OutlineComponent.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType CE::PostPrOutlineComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<PostPrOutlineComponent>{}, "PostPrOutlineComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&PostPrOutlineComponent::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&PostPrOutlineComponent::mThickness, "mThickness").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&PostPrOutlineComponent::mBias, "mBias").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<PostPrOutlineComponent>(metaType);
	return metaType;
}
