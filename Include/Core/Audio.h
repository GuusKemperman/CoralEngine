#pragma once
#include <magic_enum/magic_enum.hpp>

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
		friend EngineSubsystem;
		Audio();
		~Audio();

	public:
		void Update();

		enum class Group
		{
			Game,
			Music,
			Menu,
		};

		FMOD::System& GetCoreSystem() const { return *mCoreSystem; }
		FMOD::ChannelGroup& GetChannelGroup(Group group) const { return *mChannelGroups[static_cast<int>(group)]; }

	private:

		FMOD::System* mCoreSystem = nullptr;

		std::array<FMOD::ChannelGroup*, magic_enum::enum_count<Group>()> mChannelGroups{};
	};
}

template<>
struct Reflector<CE::Audio::Group>
{
    static CE::MetaType Reflect();
    static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(Group, CE::Audio::Group);

