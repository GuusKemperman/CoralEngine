#include "Precomp.h"
#include "Components/IsDestroyedTag.h"

#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::IsDestroyedTag::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<IsDestroyedTag>{}, "IsDestroyedTag" };
	metaType.GetProperties().Add(Props::sNoInspectTag);

	ReflectComponentType<IsDestroyedTag>(metaType);

	return metaType;
}
