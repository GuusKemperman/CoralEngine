#include "Precomp.h"
#include "Components/AudioListenerComponent.h"

#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/Reflect/ReflectComponentType.h"

#ifdef EDITOR
void CE::AudioListenerComponent::ChannelGroupControl::DisplayWidget(const std::string&)
{
	CE::ShowInspectUI("mGroup", mGroup);
	CE::ShowInspectUI("mVolume", mVolume);
	CE::ShowInspectUI("mPitch", mPitch);
}
#endif // EDITOR

bool CE::AudioListenerComponent::ChannelGroupControl::operator==(const ChannelGroupControl& other) const
{
	return mGroup == other.mGroup
		&& mVolume == other.mVolume
		&& mPitch == other.mPitch;
}

bool CE::AudioListenerComponent::ChannelGroupControl::operator!=(const ChannelGroupControl& other) const
{
	return !(*this == other);
}

CE::MetaType CE::AudioListenerComponent::ChannelGroupControl::Reflect()
{
	CE::MetaType type = CE::MetaType{ CE::MetaType::T<ChannelGroupControl>{}, "ChannelGroupControl" };
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag).Add(CE::Props::sIsScriptOwnableTag);

	type.AddField(&ChannelGroupControl::mGroup, "mGroup").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ChannelGroupControl::mVolume, "mVolume").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ChannelGroupControl::mPitch, "mPitch").GetProperties().Add(Props::sIsScriptableTag);

	CE::ReflectFieldType<ChannelGroupControl>(type);
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

CE::MetaType CE::AudioListenerComponent::Reflect()
{
	auto type = MetaType{ MetaType::T<AudioListenerComponent>{}, "AudioListenerComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	type.AddField(&AudioListenerComponent::mMasterVolume, "mMasterVolume").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&AudioListenerComponent::mMasterPitch, "mMasterPitch").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&AudioListenerComponent::mUseLowPass, "mUseLowPass").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&AudioListenerComponent::mChannelGroupControls, "mChannelGroupControls").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<AudioListenerComponent>(type);

	return type;
}

CE::MetaType CE::AudioListenerSelectedTag::Reflect()
{
	MetaType type = MetaType{ MetaType::T<AudioListenerSelectedTag>{}, "AudioListenerSelectedTag" };
	type.GetProperties().Add(Props::sNoInspectTag);

	ReflectComponentType<AudioListenerSelectedTag>(type);

	return type;
}