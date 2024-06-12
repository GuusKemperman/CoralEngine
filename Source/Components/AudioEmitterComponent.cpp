#include "Precomp.h"
#include "Components/AudioEmitterComponent.h"

#include <fmod/fmod.hpp>
#include "Assets/Sound.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::AudioEmitterComponent::Play()
{
	mShouldStartPlay = true;

	if (mSound != nullptr)
	{
		uint32 hash = GetSoundNameHash();

		if (mPlayingOnChannels.find(hash) == mPlayingOnChannels.end())
		{
			FMOD::Channel* channel = mSound->Play();

			if (channel != nullptr)
			{
				mPlayingOnChannels.emplace(hash, channel);
			}
		}
	}
}

void CE::AudioEmitterComponent::SetLoops(int loops)
{
	if (mSound != nullptr)
	{
		uint32 hash = GetSoundNameHash();

		auto it = mPlayingOnChannels.find(hash);
		if (it != mPlayingOnChannels.end())
		{
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

void CE::AudioEmitterComponent::SetPause(bool pause)
{
	if (mSound != nullptr)
	{
		uint32 hash = GetSoundNameHash();

		auto it = mPlayingOnChannels.find(hash);
		if (it != mPlayingOnChannels.end())
		{
			it->second->setPaused(pause);
		}
	}
}

void CE::AudioEmitterComponent::StopChannel()
{
	if (mSound != nullptr)
	{
		uint32 hash = GetSoundNameHash();

		auto it = mPlayingOnChannels.find(hash);
		if (it != mPlayingOnChannels.end())
		{
			it->second->stop();
		}
		mPlayingOnChannels.erase(it);
	}
}

uint32 CE::AudioEmitterComponent::GetSoundNameHash()
{
	return Name::HashString(mSound.GetMetaData().GetName());
}

CE::MetaType CE::AudioEmitterComponent::Reflect()
{
	auto type = MetaType{ MetaType::T<AudioEmitterComponent>{}, "AudioEmitterComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	type.AddField(&AudioEmitterComponent::mSound, "mSound").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::Play, "Play").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sCallFromEditorTag);
	type.AddFunc(&AudioEmitterComponent::SetPause, "SetPause", "", "Pause").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::SetLoops, "SetLoops", "Loops").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<AudioEmitterComponent>(type);

	return type;
}
