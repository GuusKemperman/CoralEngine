#pragma once
#include "Systems/System.h"

namespace FMOD
{
	class ChannelGroup;
	class DSP;
}

namespace CE
{
	class AudioSystem final
		: public System
	{
	public:

		AudioSystem();

		void Update(World& world, float dt) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mShouldTickBeforeBeginPlay = true;
			traits.mShouldTickWhilstPaused = true;
			return traits;
		}

	private:

		FMOD::ChannelGroup* mMasterChannelGroup{};
		FMOD::DSP* mLowPassDSP{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AudioSystem);
	};
}