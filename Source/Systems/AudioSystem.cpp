#include "Precomp.h"
#include "Systems/AudioSystem.h"

#include <fmod/fmod.hpp>
#include "World/World.h"
#include "Core/Audio.h"
#include "World/Registry.h"
#include "Components/AudioEmitterComponent.h"
#include "Meta/MetaType.h"

void CE::AudioSystem::Update(World& world, float)
{
	Registry& reg = world.GetRegistry();

	// find listener component
	// apply any filters or settings from the listenercomponent

	Audio::Get().GetCoreSystem().getMasterChannelGroup(&mMasterChannelGroup);

	auto view = reg.View<AudioEmitterComponent>();
	for (auto [entity, emitter] : view.each())
	{
		std::vector<int> channelsToRemove{};
		for (auto& channel : emitter.mPlayingOnChannels)
		{
			bool isPlaying{};
			channel.second->isPlaying(&isPlaying);

			if (!isPlaying)
			{
				channelsToRemove.push_back(channel.first);
			}
		}

		for (auto hash : channelsToRemove) emitter.mPlayingOnChannels.erase(hash);
	}
}

CE::MetaType CE::AudioSystem::Reflect()
{
	return MetaType{ MetaType::T<AudioSystem>{}, "AudioSystem", MetaType::Base<System>{} };
}
