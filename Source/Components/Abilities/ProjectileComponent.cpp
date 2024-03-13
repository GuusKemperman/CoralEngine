#include "Precomp.h"
#include "Components/Abilities/ProjectileComponent.h"

#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::ProjectileComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<ProjectileComponent>{}, "ProjectileComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&ProjectileComponent::mRange, "mRange").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&ProjectileComponent::mCurrentRange, "mCurrentRange").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&ProjectileComponent::mSpeed, "mSpeed").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<ProjectileComponent>(metaType);

	return metaType;
}
