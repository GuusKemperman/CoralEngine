#include "Precomp.h"
#include "Components/MeshColorComponent.h"

#include "Meta/MetaProps.h"
#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"


Engine::MetaType Engine::MeshColorComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<MeshColorComponent>{}, "MeshColorComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&MeshColorComponent::colorMultiplier, "mColorMultiplier").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&MeshColorComponent::colorAddition, "mColorAddition").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<MeshColorComponent>(metaType);

	return metaType;
}
