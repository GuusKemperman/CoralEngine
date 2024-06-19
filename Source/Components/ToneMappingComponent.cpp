#include "Precomp.h"
#include "Components/ToneMappingComponent.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Texture.h"

CE::MetaType CE::ToneMappingComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<ToneMappingComponent>{}, "ToneMappingComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&ToneMappingComponent::mExposure, "mExposure").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&ToneMappingComponent::mLUTtexture, "mLUTtexture").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&ToneMappingComponent::mNumberOfBlocks, "mNumberOfBlocks").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&ToneMappingComponent::mInvertLUTOnY, "mInvertLUTOnY").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<ToneMappingComponent>(metaType);

	return metaType;
}
