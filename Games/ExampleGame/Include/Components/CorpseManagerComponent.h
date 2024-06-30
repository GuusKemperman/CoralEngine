#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace Game
{
	struct CorpseManagerComponent
	{
		float mSinkSpeed = 0.2f;
		float mDestroyAtHeight = -2.5f;
		float mMaxTimeAlive = 60.0f;
		uint32 mMaxAlive = 500;

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(CorpseManagerComponent);
	};
}
