#pragma once
#include "Systems/System.h"

namespace Game
{
	class SpawnerSystem final :
		public Engine::System
	{
	public:
		void Update(Engine::World& world, float dt) override;

	private:
		friend Engine::ReflectAccess;
		static Engine::MetaType Reflect();
		REFLECT_AT_START_UP(SpawnerSystem);
	};
}
