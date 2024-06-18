#include "Precomp.h"
#include "Components/AudioEmitterComponent.h"

#include <fmod/fmod.hpp>
#include "Assets/Sound.h"
#include "Core/Audio.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::AudioEmitterComponent::Play(AssetHandle<Sound> sound)
{
	if (sound != nullptr)
	{
		uint32 hash = GetSoundNameHash(sound);

		FMOD::Channel* channel = sound->Play(mGroup);

		if (channel != nullptr)
		{
			mPlayingOnChannels.emplace(hash, channel);
		}
	}
}

void CE::AudioEmitterComponent::SetLoops(AssetHandle<Sound> sound, int loops)
{
	if (sound != nullptr)
	{
		uint32 hash = GetSoundNameHash(sound);

		auto it = mPlayingOnChannels.find(hash);
		if (it != mPlayingOnChannels.end())
		{
			FMOD_MODE mode{};
			FMOD_RESULT result = it->second->getMode(&mode);
			if (result != FMOD_OK)
			{
				LOG(LogAudio, Error, "FMOD Channel mode could not be retrieved, FMOD error {}", static_cast<int>(result));
			}

			mode = mode | FMOD_LOOP_NORMAL;

			result = it->second->setMode(mode);
			if (result != FMOD_OK)
			{
				LOG(LogAudio, Error, "FMOD Channel mode could not be set, FMOD error {}", static_cast<int>(result));
			}

			result = it->second->setLoopCount(loops);
			if (result != FMOD_OK)
			{
				LOG(LogAudio, Error, "FMOD Channel loop count could not be set, FMOD error {}", static_cast<int>(result));
			}
		}
	}
}

void CE::AudioEmitterComponent::StopAll()
{
	for (auto& channel : mPlayingOnChannels)
	{
		const FMOD_RESULT result = channel.second->stop();
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD Channel could not be stopped, FMOD error {}", static_cast<int>(result));
		}
	}

	mPlayingOnChannels.clear();
}

void CE::AudioEmitterComponent::SetPause(AssetHandle<Sound> sound, bool pause)
{
	if (sound != nullptr)
	{
		uint32 hash = GetSoundNameHash(sound);

		auto it = mPlayingOnChannels.find(hash);
		if (it != mPlayingOnChannels.end())
		{
			const FMOD_RESULT result = it->second->setPaused(pause);
			if (result != FMOD_OK)
			{
				LOG(LogAudio, Error, "FMOD Channel could not be paused, FMOD error {}", static_cast<int>(result));
			}
		}
	}
}

void CE::AudioEmitterComponent::Stop(AssetHandle<Sound> sound)
{
	if (sound != nullptr)
	{
		uint32 hash = GetSoundNameHash(sound);

		auto it = mPlayingOnChannels.find(hash);
		if (it != mPlayingOnChannels.end())
		{
			FMOD_RESULT result = it->second->stop();
			if (result != FMOD_OK)
			{
				LOG(LogAudio, Error, "FMOD Channel could not be stopped, FMOD error {}", static_cast<int>(result));
			}
		}
		mPlayingOnChannels.erase(it);
	}
}

void CE::AudioEmitterComponent::SetChannelGroup(Audio::Group group)
{
	if (group == mGroup)
	{
		return;
	}

	mGroup = group;

	for (auto& channel : mPlayingOnChannels)
	{
		FMOD_RESULT result = channel.second->setChannelGroup(&Audio::Get().GetChannelGroup(group));
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD Channel was unable to set channel group, FMOD error {}", static_cast<int>(result));
		}
	}
}

void CE::AudioEmitterComponent::OnEndPlay(World& world, entt::entity owner)
{
	AudioEmitterComponent* emitter = world.GetRegistry().TryGet<AudioEmitterComponent>(owner);

	if (emitter != nullptr)
	{
		emitter->StopAll();
	}
}

uint32 CE::AudioEmitterComponent::GetSoundNameHash(AssetHandle<Sound> sound)
{
	return Name::HashString(sound.GetMetaData().GetName());
}

CE::MetaType CE::AudioEmitterComponent::Reflect()
{
	auto type = MetaType{ MetaType::T<AudioEmitterComponent>{}, "AudioEmitterComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	BindEvent(type, CE::sEndPlayEvent, &AudioEmitterComponent::OnEndPlay);
	
	type.AddField(&AudioEmitterComponent::mGroup, "mGroup").GetProperties().Add(Props::sIsEditorReadOnlyTag).Add(Props::sIsScriptReadOnlyTag);

	// Script functions
	type.AddFunc(&AudioEmitterComponent::Play, "Play", "", "Sound").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::SetPause, "SetPause", "", "Sound", "Pause").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::SetLoops, "SetLoops", "", "Sound", "Loops").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::StopAll, "StopAll").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::Stop, "Stop", "", "Sound").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::SetChannelGroup, "SetChannnelGroup", "", "Group").GetProperties().Add(Props::sIsScriptableTag);


	ReflectComponentType<AudioEmitterComponent>(type);

	return type;
}
