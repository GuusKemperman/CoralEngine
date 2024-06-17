#include "Precomp.h"
#include "Components/AudioEmitterComponent.h"

#include <fmod/fmod.hpp>
#include "Assets/Sound.h"
#include "Core/Audio.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::AudioEmitterComponent::Play(AssetHandle<Sound> sound)
{
	mShouldStartPlay = true;

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
			it->second->getMode(&mode);

			mode = mode | FMOD_LOOP_NORMAL;

			it->second->setMode(FMOD_LOOP_NORMAL);
			it->second->setLoopCount(loops);
		}
	}
}

void CE::AudioEmitterComponent::StopAll()
{
	for (auto& channel : mPlayingOnChannels)
	{
		channel.second->stop();
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
			it->second->setPaused(pause);
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
			it->second->stop();
		}
		mPlayingOnChannels.erase(it);
	}
}

void CE::AudioEmitterComponent::Set3DAttributes(glm::vec3* position, glm::vec3* velocity)
{
	for (auto& channel : mPlayingOnChannels)
	{
		channel.second->set3DAttributes(reinterpret_cast<FMOD_VECTOR*>(position), reinterpret_cast<FMOD_VECTOR*>(velocity));
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
		channel.second->setChannelGroup(&Audio::Get().GetChannelGroup(group));
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

	//type.AddField(&AudioEmitterComponent::mSound, "mSound").GetProperties().Add(Props::sIsScriptableTag);
	
	type.AddField(&AudioEmitterComponent::mGroup, "mGroup").GetProperties().Add(Props::sIsEditorReadOnlyTag).Add(Props::sIsScriptReadOnlyTag);

	// Script functions
	type.AddFunc(&AudioEmitterComponent::Play, "Play", "", "Sound").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::SetPause, "SetPause", "", "Sound", "Pause").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::SetLoops, "SetLoops", "", "Sound", "Loops").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::StopAll, "StopAll").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::Stop, "Stop", "", "Sound").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::Set3DAttributes, "Set3DAttributes", "", "Position", "Velocity").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::SetChannelGroup, "SetChannnelGroup", "", "Group").GetProperties().Add(Props::sIsScriptableTag);


	ReflectComponentType<AudioEmitterComponent>(type);

	return type;
}
