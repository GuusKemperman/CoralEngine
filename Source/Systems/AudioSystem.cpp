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

	/*result = mMasterChannelGroup->addDSP(0, mLowPassDSP);
	if (result != FMOD_OK)
	{
		LOG(LogAudio, Error, "FMOD was unable to add DSP");
	}*/
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


		FMOD_RESULT result = mMasterChannelGroup->setVolume(listenerComponent.mVolume);
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD could not set master channel group volume, FMOD error {}", static_cast<int>(result));
		}
		
		result = mMasterChannelGroup->setPitch(listenerComponent.mPitch);
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD could not set master channel group pitch, FMOD error {}", static_cast<int>(result));
		}

		if (listenerComponent.mUseLowPass)
		{
			mMasterChannelGroup->addDSP(0, mLowPassDSP);
		}

		// if 3d sound
		{
			const TransformComponent* transform = reg.TryGet<TransformComponent>(listenerOwner);

			if (transform != nullptr)
			{
				glm::vec3 position = transform->GetWorldPosition();
				glm::vec3 forward = ToVector3(Axis::Forward);
				glm::vec3 up = ToVector3(Axis::Up);

				result = Audio::Get().GetCoreSystem().set3DListenerAttributes(0, 
					reinterpret_cast<FMOD_VECTOR*>(&position), 
					nullptr, 
					reinterpret_cast<FMOD_VECTOR*>(&forward), 
					reinterpret_cast<FMOD_VECTOR*>(&up));
				if (result != FMOD_OK)
				{
					LOG(LogAudio, Error, "FMOD could not set listener 3D attributes, FMOD error {}", static_cast<int>(result));
				}

				result = Audio::Get().GetCoreSystem().get3DListenerAttributes(0, reinterpret_cast<FMOD_VECTOR*>(&position), 
					nullptr, 
					reinterpret_cast<FMOD_VECTOR*>(&forward), 
					reinterpret_cast<FMOD_VECTOR*>(&up));
				if (result != FMOD_OK)
				{
					LOG(LogAudio, Error, "FMOD could not Retrieve listener 3D attributes, FMOD error {}", static_cast<int>(result));
				}

				LOG(LogAudio, Message, "Listener Position: {},{},{}, Forward: {},{},{}, Up: {},{},{}", position.x, position.y, position.z, forward.x, forward.y, forward.z, up.x, up.y, up.z);
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

		// Set 3d position for game audio
		if (emitter.mGroup == Audio::Group::Game)
		{
			const TransformComponent* transform = reg.TryGet<TransformComponent>(entity);

			if (transform != nullptr)
			{
				glm::vec3 position = transform->GetWorldPosition();
				emitter.Set3DAttributes(position, glm::vec3(0.f));
			}
		}
	}
}

CE::MetaType CE::AudioSystem::Reflect()
{
	return MetaType{ MetaType::T<AudioSystem>{}, "AudioSystem", MetaType::Base<System>{} };
}
