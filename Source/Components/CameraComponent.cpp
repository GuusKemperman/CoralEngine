#include "Precomp.h"
#include "Components/CameraComponent.h" 

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

entt::entity CE::CameraComponent::GetSelected(const World& world)
{
	World& mutWorld = const_cast<World&>(world);
	Registry& reg = mutWorld.GetRegistry();
	entt::entity selectedEntity = reg.View<CameraComponent, CameraSelectedTag>().front();

	if (selectedEntity == entt::null)
	{
		Deselect(mutWorld);
		selectedEntity = reg.View<CameraComponent>().front();

		if (selectedEntity != entt::null)
		{
			Select(mutWorld, selectedEntity);
		}
	}

	return selectedEntity;
}

bool CE::CameraComponent::IsSelected(const World& world, entt::entity cameraOwner)
{
	return world.GetRegistry().HasComponent<CameraSelectedTag>(cameraOwner);
}

void CE::CameraComponent::Select(World& world, entt::entity cameraOwner)
{
	Deselect(world);
	world.GetRegistry().AddComponent<CameraSelectedTag>(cameraOwner);
}

void CE::CameraComponent::Deselect(World& world)
{
	const auto view = world.GetRegistry().View<CameraSelectedTag>();

	std::vector<entt::entity> entitiesWithTag{};
	entitiesWithTag.reserve(view.size());

	for (auto [entity] : view.each())
	{
		entitiesWithTag.emplace_back(entity);

	}

	for (const entt::entity entity : entitiesWithTag)
	{
		world.GetRegistry().RemoveComponent<CameraSelectedTag>(entity);
	}
}

CE::MetaType CE::CameraComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<CameraComponent>{}, "CameraComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&CameraComponent::mFar, "mFar").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&CameraComponent::mNear, "mNear").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&CameraComponent::mFOV, "mFOV").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<CameraComponent>(type);

	return type;
}

CE::MetaType CE::CameraSelectedTag::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<CameraSelectedTag>{}, "CameraSelectedTag" };
	metaType.GetProperties().Add(Props::sNoInspectTag);

	ReflectComponentType<CameraSelectedTag>(metaType);

	return metaType;
}
