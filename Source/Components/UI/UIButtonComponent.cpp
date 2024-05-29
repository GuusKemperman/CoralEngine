#include "Precomp.h"
#include "Components/UI/UIButtonComponent.h"

#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Registry.h"
#include "World/World.h"

CE::MetaType CE::UIButtonSelectedTag::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<UIButtonSelectedTag>{}, "UIButtonSelectedTag" };
	ReflectComponentType<UIButtonSelectedTag>(metaType);

	return metaType;
}

void CE::UIButtonTag::Select(World& world, entt::entity buttonOwner)
{
	Deselect(world);
	world.GetRegistry().AddComponent<UIButtonSelectedTag>(buttonOwner);
}

void CE::UIButtonTag::Deselect(World& world)
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

bool CE::UIButtonTag::IsSelected(const World& world, entt::entity buttonOwner)
{
	return world.GetRegistry().HasComponent<UIButtonSelectedTag>(buttonOwner);
}

CE::MetaType CE::UIButtonTag::Reflect()
{
	MetaType type = MetaType{ MetaType::T<UIButtonTag>{}, "UIButtonTag" };

	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	props.Set(Props::sOldNames, "UIButtonComponent");

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

	ReflectComponentType<UIButtonTag>(type);

	return type;
}
