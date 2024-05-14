#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class Sound;
	class World;

	class AudioEmitterComponent
	{
	public:
		void Play() const;

		AssetHandle<Sound> mSound{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AudioEmitterComponent);
	};
}