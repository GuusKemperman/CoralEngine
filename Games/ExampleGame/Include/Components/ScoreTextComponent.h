#pragma once
#include "Meta/MetaReflect.h"
#include "Utilities/Events.h"

namespace CE
{
	class World;
}

namespace Game
{

	class ScoreTextComponent
	{
	public:
		void OnTick(CE::World& world, entt::entity owner, float dt) const;

		bool mDisplayHighScore = false;

	private:

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(ScoreTextComponent);
	};

}
