#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"

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
		void Play();
		void SetLoops(int loops);
		void StopAll();
		void SetPause(bool pause);
		void StopChannel();

		AssetHandle<Sound> mSound{};

		std::unordered_map<uint32, FMOD::Channel*> mPlayingOnChannels{};

		bool mShouldStartPlay = false;
		bool mIsLooping = false;

		float mVolume = 1.0f;

	private:
		uint32 GetSoundNameHash();

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AudioEmitterComponent);
	};
}