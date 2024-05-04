#include "Precomp.h"
#include "Components/IsAwaitingBeginPlayTag.h"

#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType CE::IsAwaitingBeginPlayTag::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<IsAwaitingBeginPlayTag>{}, "IsAwaitingBeginPlayTag" };
	metaType.GetProperties().Add(Props::sNoInspectTag).Add(Props::sNoSerializeTag);

	ReflectComponentType<IsAwaitingBeginPlayTag>(metaType);

	return metaType;
}
