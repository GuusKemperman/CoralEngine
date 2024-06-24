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
		void OnTick(CE::World& world, entt::entity owner, float dt);

		int CheckHighScore() const;

		int mTotalScore = 0;
		static inline int mHighScore = 0;

		int mMaxTotalScore = 999999;
	private:

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(ScoreComponent);
	};

}
