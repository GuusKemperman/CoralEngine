#include "Precomp.h"
#include "Components/NameComponent.h"

#include "Meta/MetaProps.h"
#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Registry.h"

std::string_view CE::NameComponent::GetDisplayName(const Registry& registry, entt::entity entity)
{
	const NameComponent* nameComponent = registry.TryGet<const NameComponent>(entity);
	return GetDisplayName(nameComponent, entity);
}

std::string_view CE::NameComponent::GetDisplayName(const NameComponent* nameComponent, [[maybe_unused]] entt::entity entity)
{
	return nameComponent == nullptr ? std::string_view{ "Unnamed entity" } : std::string_view{ nameComponent->mName };
}

CE::MetaType CE::NameComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<NameComponent>{}, "NameComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&NameComponent::mName, "mName").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<NameComponent>(metaType);

	return metaType;
}
