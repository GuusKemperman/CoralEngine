#include "Precomp.h"
#include "Components/UIButtonComponent.h"

#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Registry.h"
#include "World/World.h"

Engine::MetaType Engine::UIButtonSelectedTag::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<UIButtonSelectedTag>{}, "UIButtonSelectedTag" };
	ReflectComponentType<UIButtonSelectedTag>(metaType);

	return metaType;
}

void Engine::UIButtonComponent::Select(World& world, entt::entity buttonOwner)
{
	Deselect(world);
	world.GetRegistry().AddComponent<UIButtonSelectedTag>(buttonOwner);
}

void Engine::UIButtonComponent::Deselect(World& world)
{
	auto view = world.GetRegistry().View<UIButtonSelectedTag>();

	std::vector<entt::entity> entitiesWithTag{};
	entitiesWithTag.reserve(view.size());

	for (auto [entity] : view.each())
	{
		entitiesWithTag.emplace_back(entity);

	}

	for (const entt::entity entity : entitiesWithTag)
	{
		world.GetRegistry().RemoveComponent<UIButtonSelectedTag>(entity);
	}
}

Engine::MetaType Engine::UIButtonComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<UIButtonComponent>{}, "UIButtonComponent" };

	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	type.AddField(&UIButtonComponent::mButtonTopSide, "mButtonTopSide").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&UIButtonComponent::mButtonBottomSide, "mButtonBottomSide").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&UIButtonComponent::mButtonRightSide, "mButtonRightSide").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&UIButtonComponent::mButtonLeftSide, "mButtonLeftSide").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<UIButtonComponent>(type);

	return type;
}
