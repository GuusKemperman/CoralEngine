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
			Game = 0,
			Music = 1,
			Menu = 2
		};

		FMOD::System& GetCoreSystem() const { return *mCoreSystem; }
		FMOD::ChannelGroup& GetChannelGroup(Group group) const { return *mChannelGroups[static_cast<int>(group)]; }

	private:

		FMOD::System* mCoreSystem = nullptr;

		std::vector<FMOD::ChannelGroup*> mChannelGroups{3, nullptr};
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
    static constexpr EnumStringPairs<Audio::Group, 3> value = {
        EnumStringPair<Audio::Group>
		{ Audio::Group::Music, "Music" },
        { Audio::Group::Game, "Game" },
        { Audio::Group::Menu, "Menu" },
    };
};
