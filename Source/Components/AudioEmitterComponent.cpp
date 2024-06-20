#include "Precomp.h"
#include "Components/AudioEmitterComponent.h"

#include <fmod/fmod.hpp>
#include "Assets/Sound.h"
#include "Core/Audio.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::AudioEmitterComponent& CE::AudioEmitterComponent::operator=(AudioEmitterComponent&& other) noexcept
{
	if (this != &other)
	{
		mPlayingOnChannels = std::move(other.mPlayingOnChannels);
		mGroup = other.mGroup;

#ifdef EDITOR
		mSound = other.mSound;
		mVolume = other.mVolume;
		mPitch = other.mPitch;
#endif // EDITOR
	}

	return *this;
}

CE::AudioEmitterComponent::~AudioEmitterComponent()
{
	StopAll();
}

void CE::AudioEmitterComponent::Play(AssetHandle<Sound> sound, float volume, float pitch)
{
	if (sound == nullptr)
	{
		return;
	}

	FMOD::Channel* channel = sound->Play(mGroup);

	if (channel == nullptr)
	{
		return;
	}

	Name::HashType hash = GetSoundNameHash(sound);

	FMOD_RESULT result = channel->setVolume(volume);
	if (result != FMOD_OK)
	{
		LOG(LogAudio, Error, "FMOD could not set channel volume, FMOD error {}", static_cast<int>(result));
	}
		
	result = channel->setPitch(pitch);
	if (result != FMOD_OK)
	{
		LOG(LogAudio, Error, "FMOD could not set channel pitch, FMOD error {}", static_cast<int>(result));
	}

	result = channel->setLoopCount(0);
	if (result != FMOD_OK)
	{
		LOG(LogAudio, Error, "FMOD could not set channel loops, FMOD error {}", static_cast<int>(result));
	}

	if (channel != nullptr)
	{
		mPlayingOnChannels.emplace(hash, channel);
	}
}

void CE::AudioEmitterComponent::SetLoops(AssetHandle<Sound> sound, int loops)
{
	if (sound == nullptr)
	{
		return;
	}

	Name::HashType hash = GetSoundNameHash(sound);

	auto it = mPlayingOnChannels.find(hash);
	if (it != mPlayingOnChannels.end())
	{
		const FMOD_RESULT result = it->second->setLoopCount(loops);
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD Channel loop count could not be set, FMOD error {}", static_cast<int>(result));
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
	if (sound == nullptr)
	{
		return;
	}

	Name::HashType hash = GetSoundNameHash(sound);

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

void CE::AudioEmitterComponent::Stop(AssetHandle<Sound> sound)
{
	if (sound == nullptr)
	{
		return;
	}

	Name::HashType hash = GetSoundNameHash(sound);

	auto it = mPlayingOnChannels.find(hash);
	if (it != mPlayingOnChannels.end())
	{
		FMOD_RESULT result = it->second->stop();
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD Channel could not be stopped, FMOD error {}", static_cast<int>(result));
		}
		mPlayingOnChannels.erase(it);
	}
}

void CE::AudioEmitterComponent::SetVolume(AssetHandle<Sound> sound, float volume)
{
	if (sound == nullptr)
	{
		return;
	}

	Name::HashType hash = GetSoundNameHash(sound);

	auto it = mPlayingOnChannels.find(hash);
	if (it != mPlayingOnChannels.end())
	{
		FMOD_RESULT result = it->second->setVolume(volume);
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD Channel volume could not be set, FMOD error {}", static_cast<int>(result));
		}
	}
}

void CE::AudioEmitterComponent::SetPitch(AssetHandle<Sound> sound, float pitch)
{
	if (sound == nullptr)
	{
		return;
	}
	Name::HashType hash = GetSoundNameHash(sound);

	auto it = mPlayingOnChannels.find(hash);
	if (it != mPlayingOnChannels.end())
	{
		FMOD_RESULT result = it->second->setPitch(pitch);
		if (result != FMOD_OK)
		{
			LOG(LogAudio, Error, "FMOD Channel pitch could not be set, FMOD error {}", static_cast<int>(result));
		}
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

uint32 CE::AudioEmitterComponent::GetSoundNameHash(AssetHandle<Sound> sound)
{
	return Name::HashString(sound.GetMetaData().GetName());
}

CE::MetaType CE::AudioEmitterComponent::Reflect()
{
	auto type = MetaType{ MetaType::T<AudioEmitterComponent>{}, "AudioEmitterComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	
	// Editor only functions and fields for testing
#ifdef EDITOR
	type.AddField(&AudioEmitterComponent::mGroup, "mGroup");
	type.AddField(&AudioEmitterComponent::mSound, "mSound");
	type.AddField(&AudioEmitterComponent::mVolume, "mVolume");
	type.AddField(&AudioEmitterComponent::mPitch, "mPitch");
	
	type.AddFunc([](AudioEmitterComponent& audioEmitter)
		{
			audioEmitter.Play(audioEmitter.mSound, audioEmitter.mVolume, audioEmitter.mPitch);
		}, "Play Sound", MetaFunc::ExplicitParams<AudioEmitterComponent&>{}
		).GetProperties().Add(Props::sCallFromEditorTag);

	type.AddFunc(&AudioEmitterComponent::StopAll, "StopAll").GetProperties().Add(Props::sCallFromEditorTag);
	
	type.AddFunc([](AudioEmitterComponent& audioEmitter)
			{
				audioEmitter.SetChannelGroup(audioEmitter.mGroup);
			}, "Set Channel Group", MetaFunc::ExplicitParams<AudioEmitterComponent&>{}
			).GetProperties().Add(Props::sCallFromEditorTag);
#endif // EDITOR

	// Script functions
	type.AddFunc(&AudioEmitterComponent::Play, "Play", "", "Sound", "Volume", "Pitch").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::SetVolume, "SetVolume", "", "Volume").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::SetPitch, "SetPitch", "", "Pitch").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::SetPause, "SetPause", "", "Sound", "Pause").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::SetLoops, "SetLoops", "", "Sound", "Loops").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::StopAll, "StopAll").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::Stop, "Stop", "", "Sound").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::SetChannelGroup, "SetChannnelGroup", "", "Group").GetProperties().Add(Props::sIsScriptableTag);


	ReflectComponentType<AudioEmitterComponent>(type);

	return type;
}
