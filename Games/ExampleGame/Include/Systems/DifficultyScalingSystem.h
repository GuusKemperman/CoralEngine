#pragma once
#include "Systems/System.h"

namespace Game
{
	class DifficultyScalingSystem final :
		public CE::System
	{
	public:
		void Update(CE::World& world, float dt) override;

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(DifficultyScalingSystem);
	};
}
