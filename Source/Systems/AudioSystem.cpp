#include "Precomp.h"
#include "Systems/AudioSystem.h"

#include <fmod/fmod.hpp>
#include "World/World.h"
#include "Core/Audio.h"
#include "World/Registry.h"
#include "Components/AudioEmitterComponent.h"
#include "Components/AudioListenerComponent.h"
#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"

void CE::AudioSystem::Update(World& world, float)
{
	Registry& reg = world.GetRegistry();

	entt::entity listenerOwner = AudioListenerComponent::GetSelected(world);

	if (listenerOwner != entt::null)
	{
		AudioListenerComponent& listenerComponent = reg.Get<AudioListenerComponent>(listenerOwner);
		
		FMOD::ChannelGroup* masterChannelGroup; 
		FMOD_RESULT result = Audio::Get().GetCoreSystem().getMasterChannelGroup(&masterChannelGroup);
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD could not retrieve master channel group, FMOD error {}", static_cast<int>(result));
			return;
		}

		result = masterChannelGroup->setVolume(listenerComponent.mMasterVolume);
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD could not set master channel group volume, FMOD error {}", static_cast<int>(result));
		}
		
		result = masterChannelGroup->setPitch(listenerComponent.mMasterPitch);
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD could not set master channel group pitch, FMOD error {}", static_cast<int>(result));
		}

		for (auto& channelGroupControl : listenerComponent.mChannelGroupControls)
		{
			FMOD::ChannelGroup& channelGroup = Audio::Get().GetChannelGroup(channelGroupControl.mGroup);
			
			result = channelGroup.setVolume(channelGroupControl.mVolume);
			if (result != FMOD_OK)
			{
				LOG(LogAudio, Error, "FMOD could not set channel group volume, FMOD error {}", static_cast<int>(result));
			}

			result = channelGroup.setPitch(channelGroupControl.mPitch);
			if (result != FMOD_OK)
			{
				LOG(LogAudio, Error, "FMOD could not set channel group pitch, FMOD error {}", static_cast<int>(result));
			}
		}
	}

	{
		bool isPaused = false;

		FMOD::ChannelGroup& channelGroup = Audio::Get().GetChannelGroup(Audio::Group::Game);

		FMOD_RESULT result = channelGroup.getPaused(&isPaused);
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD could not get channel group pause, FMOD error {}", static_cast<int>(result));
		}

		if (world.IsPaused() != isPaused)
		{
			result = channelGroup.setPaused(!isPaused);
			if (result != FMOD_OK)
			{
				LOG(LogAudio, Error, "FMOD could not set channel group pause, FMOD error {}", static_cast<int>(result));
			}
		}
	}
	
	auto view = reg.View<AudioEmitterComponent>();
	for (auto [entity, emitter] : view.each())
	{
		std::vector<Name::HashType> channelsToRemove{};
		for (auto& channel : emitter.mPlayingOnChannels)
		{
			bool isPlaying{};
			FMOD_RESULT result = channel.second->isPlaying(&isPlaying);
			if (result != FMOD_OK)
			{
				LOG(LogAudio, Error, "FMOD could not find out if channel is playing, FMOD error {}", static_cast<int>(result));
			}

			if (!isPlaying)
			{
				result = channel.second->stop();
				if (result != FMOD_OK)
				{
					LOG(LogAudio, Error, "FMOD could not stop channel, FMOD error {}", static_cast<int>(result));
				}
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
