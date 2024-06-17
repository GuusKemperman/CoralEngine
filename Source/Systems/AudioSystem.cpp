#include "Precomp.h"
#include "Systems/AudioSystem.h"

#include <fmod/fmod.hpp>
#include "World/World.h"
#include "Core/Audio.h"
#include "World/Registry.h"
#include "Components/AudioEmitterComponent.h"
#include "Components/AudioListenerComponent.h"
#include "Meta/MetaType.h"

CE::AudioSystem::AudioSystem()
{
	Audio::Get().GetCoreSystem().getMasterChannelGroup(&mMasterChannelGroup);
	
	FMOD_RESULT result = Audio::Get().GetCoreSystem().createDSPByType(FMOD_DSP_TYPE_LOWPASS, &mLowPassDSP);
	if (result != FMOD_OK)
	{
		LOG(LogAudio, Error, "FMOD was unable to create DSP");
	}

	result = mMasterChannelGroup->addDSP(0, mLowPassDSP);
	if (result != FMOD_OK)
	{
		LOG(LogAudio, Error, "FMOD was unable to add DSP");
	}
}

void CE::AudioSystem::Update(World& world, float)
{
	Registry& reg = world.GetRegistry();

	// find listener component
	// apply any filters or settings from the listenercomponent

	entt::entity listenerOwner = AudioListenerComponent::GetSelected(world);

	if (listenerOwner != entt::null)
	{
		AudioListenerComponent& listenerComponent = reg.Get<AudioListenerComponent>(listenerOwner);
		
		// handle dsp etc


		mMasterChannelGroup->setVolume(listenerComponent.mVolume);
	}

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
