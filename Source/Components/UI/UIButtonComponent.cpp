#include "Precomp.h"
#include "Components/UI/UIButtonComponent.h"

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

bool Engine::UIButtonComponent::IsSelected(const World& world, entt::entity buttonOwner)
{
	return world.GetRegistry().HasComponent<UIButtonSelectedTag>(buttonOwner);
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

	type.AddFunc(
		[](entt::entity buttonOwner)
		{
			const World& world = *World::TryGetWorldAtTopOfStack();

			return IsSelected(world, buttonOwner);
		}, "IsSelected", MetaFunc::ExplicitParams<entt::entity>{}, "ButtonOwner").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc(
		[](entt::entity buttonOwner)
		{
			World& world = *World::TryGetWorldAtTopOfStack();
			Select(world, buttonOwner);
		}, "Select", MetaFunc::ExplicitParams<entt::entity>{}, "ButtonOwner").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc(
		[]
		{
			World& world = *World::TryGetWorldAtTopOfStack();
			Deselect(world);
		}, "Deselect").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	ReflectComponentType<UIButtonComponent>(type);

	return type;
}
