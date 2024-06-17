#include "Precomp.h"
#include "Components/AudioListenerComponent.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType CE::AudioListenerComponent::Reflect()
{
	auto type = MetaType{ MetaType::T<AudioListenerComponent>{}, "AudioListenerComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	type.AddField(&AudioListenerComponent::mVolume, "mVolume").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<AudioListenerComponent>(type);

	return type;
}

entt::entity CE::AudioListenerComponent::GetSelected(const World& world)
{
	World& mutWorld = const_cast<World&>(world);
	Registry& reg = mutWorld.GetRegistry();
	entt::entity selectedEntity = reg.View<AudioListenerComponent, AudioListenerSelectedTag>().front();

	if (selectedEntity == entt::null)
	{
		Deselect(mutWorld);
		selectedEntity = reg.View<AudioListenerComponent>().front();

		if (selectedEntity != entt::null)
		{
			Select(mutWorld, selectedEntity);
		}
	}

	return selectedEntity;
}

bool CE::AudioListenerComponent::IsSelected(const World& world, entt::entity audioListenerOwner)
{
	return world.GetRegistry().HasComponent<AudioListenerComponent>(audioListenerOwner);
}

void CE::AudioListenerComponent::Select(World& world, entt::entity audioListenerOwner)
{
	Deselect(world);
	world.GetRegistry().AddComponent<AudioListenerSelectedTag>(audioListenerOwner);
}

void CE::AudioListenerComponent::Deselect(World& world)
{
	const auto view = world.GetRegistry().View<AudioListenerSelectedTag>();

	std::vector<entt::entity> entitiesWithTag{};
	entitiesWithTag.reserve(view.size());

	for (auto [entity] : view.each())
	{
		entitiesWithTag.emplace_back(entity);

	}

	for (const entt::entity entity : entitiesWithTag)
	{
		world.GetRegistry().RemoveComponent<AudioListenerSelectedTag>(entity);
	}
}

CE::MetaType CE::AudioListenerSelectedTag::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AudioListenerSelectedTag>{}, "AudioListenerSelectedTag" };
	metaType.GetProperties().Add(Props::sNoInspectTag);

	ReflectComponentType<AudioListenerSelectedTag>(metaType);

	return metaType;
}

