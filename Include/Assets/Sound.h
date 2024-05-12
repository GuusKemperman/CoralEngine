#pragma once
#include "Assets/Asset.h"


namespace FMOD
{
	class Sound;
}

namespace CE
{
	class Sound final :
		public Asset
	{
	public:
		Sound(std::string_view name);
		Sound(AssetLoadInfo& loadInfo);

		~Sound();

		Sound(Sound&&) noexcept = delete;
		Sound(const Sound&) = delete;

		Sound& operator=(Sound&&) = delete;
		Sound& operator=(const Sound&) = delete;

		void Play() const;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Sound);

		FMOD::Sound* mSound{};
	};
}
