#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"
#include "Core/Audio.h"

namespace FMOD
{
	class Channel;
}

namespace CE
{
	class Sound;
	class World;

	class AudioEmitterComponent
	{
	public:
		void Play(AssetHandle<Sound> sound);
		void SetLoops(AssetHandle<Sound> sound, int loops);
		void StopAll();
		void SetPause(AssetHandle<Sound> sound, bool pause);
		void Stop(AssetHandle<Sound> sound);
		void SetChannelGroup(Audio::Group group);

		std::unordered_map<uint32, FMOD::Channel*> mPlayingOnChannels{};

		Audio::Group mGroup = Audio::Group::Game;

#ifdef EDITOR

		AssetHandle<Sound> mSound{};
		float mVolume;
		float mPitch;

#endif // EDITOR

	private:
		void OnEndPlay(World& world, entt::entity owner);
		uint32 GetSoundNameHash(AssetHandle<Sound> sound);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AudioEmitterComponent);
	};
}