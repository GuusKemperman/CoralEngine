#pragma once
#include "Meta/MetaReflect.h"
#include "Core/EngineSubsystem.h"

namespace FMOD
{
	class System;
	class ChannelGroup;
}

namespace CE
{
	class Audio :
		public EngineSubsystem<Audio>
	{
		friend class EngineSubsystem<Audio>;
		Audio();
		~Audio();

	public:
		void Update();

		enum class Group
		{
			Game,
			Music,
			Menu,
			NUM_OF_GROUPS
		};

		FMOD::System& GetCoreSystem() const { return *mCoreSystem; }
		FMOD::ChannelGroup& GetChannelGroup(Group group) const { return *mChannelGroups[static_cast<int>(group)]; }

	private:

		FMOD::System* mCoreSystem = nullptr;

		std::array<FMOD::ChannelGroup*,static_cast<int>(Group::NUM_OF_GROUPS)> mChannelGroups;
	};
}

template<>
struct Reflector<CE::Audio::Group>
{
    static CE::MetaType Reflect();
    static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(Group, CE::Audio::Group);

template<>
struct CE::EnumStringPairsImpl<CE::Audio::Group>
{
    static constexpr EnumStringPairs<Audio::Group, static_cast<size_t>(Audio::Group::NUM_OF_GROUPS)> value = {
        EnumStringPair<Audio::Group>
		{ Audio::Group::Music, "Music" },
        { Audio::Group::Game, "Game" },
        { Audio::Group::Menu, "Menu" },
    };
};
