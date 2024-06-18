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

CE::AudioSystem::AudioSystem()
{
	Audio::Get().GetCoreSystem().getMasterChannelGroup(&mMasterChannelGroup);
	
	const FMOD_RESULT result = Audio::Get().GetCoreSystem().createDSPByType(FMOD_DSP_TYPE_LOWPASS, &mLowPassDSP);
	if (result != FMOD_OK)
	{
		LOG(LogAudio, Error, "FMOD could not create DSP, FMOD error {}", static_cast<int>(result));
	}
}

void CE::AudioSystem::Update(World& world, float)
{
	Registry& reg = world.GetRegistry();

	entt::entity listenerOwner = AudioListenerComponent::GetSelected(world);

	if (listenerOwner != entt::null)
	{
		AudioListenerComponent& listenerComponent = reg.Get<AudioListenerComponent>(listenerOwner);
		
		FMOD_RESULT result = mMasterChannelGroup->setVolume(listenerComponent.mMasterVolume);
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD could not set master channel group volume, FMOD error {}", static_cast<int>(result));
		}
		
		result = mMasterChannelGroup->setPitch(listenerComponent.mMasterPitch);
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD could not set master channel group pitch, FMOD error {}", static_cast<int>(result));
		}

		if (listenerComponent.mUseLowPass)
		{
			mMasterChannelGroup->addDSP(0, mLowPassDSP);
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

	auto view = reg.View<AudioEmitterComponent>();
	for (auto [entity, emitter] : view.each())
	{
		std::vector<int> channelsToRemove{};
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
