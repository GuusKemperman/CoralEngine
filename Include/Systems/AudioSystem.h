#pragma once
#include "Systems/System.h"

namespace FMOD
{
	class ChannelGroup;
}

namespace CE
{
	class AudioSystem final
		: public System
	{
	public:

		void Update(World& world, float dt) override;

	private:

		FMOD::ChannelGroup* mMasterChannelGroup{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AudioSystem);
	};
}