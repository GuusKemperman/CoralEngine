#pragma once
#include "Core/EngineSubsystem.h"

namespace FMOD
{
	class System;
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

		FMOD::System& GetCoreSystem() const { return *mCoreSystem; }

	private:

		FMOD::System* mCoreSystem = nullptr;
	};
}