#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	class World;
}

namespace Game
{

	class ScoreComponent
	{
	public:

		int CheckHighScore() const;

		int mTotalScore = 0;

	private:

		static inline int mHighScore = 0;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(ScoreComponent);
	};

}
