#include "Precomp.h"
#include "Components/NameComponent.h"

#include "Meta/MetaProps.h"
#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Registry.h"

std::string Engine::NameComponent::GetDisplayName(const Registry& registry, entt::entity entity)
{
	const NameComponent* nameComponent = registry.TryGet<const NameComponent>(entity);
	return nameComponent == nullptr ? Format("Unnamed entity {}", static_cast<EntityType>(entity)) : nameComponent->mName;
}

Engine::MetaType Engine::NameComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<NameComponent>{}, "NameComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&NameComponent::mName, "mName").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<NameComponent>(metaType);

	return metaType;
}
