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
		void Play(AssetHandle<Sound> sound, float volume = 1.0f, float pitch = 1.0f);
		void SetLoops(AssetHandle<Sound> sound, int loops);
		void StopAll();
		void SetPause(AssetHandle<Sound> sound, bool pause);
		void Stop(AssetHandle<Sound> sound);
		void SetVolume(AssetHandle<Sound> sound, float volume);
		void SetPitch(AssetHandle<Sound> sound, float pitch);
		void SetChannelGroup(Audio::Group group);

		std::unordered_map<Name::HashType, FMOD::Channel*> mPlayingOnChannels{};

		Audio::Group mGroup = Audio::Group::Game;

		// Editor only, for testing
#ifdef EDITOR
		AssetHandle<Sound> mSound{};
		float mVolume = 1.0f;
		float mPitch = 1.0f;
#endif // EDITOR

	private:
		void OnEndPlay(World& world, entt::entity owner);
		uint32 GetSoundNameHash(AssetHandle<Sound> sound);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AudioEmitterComponent);
	};
}