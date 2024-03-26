#pragma once
#include "Systems/System.h"

namespace Engine
{
	class SpawnerSystem final : public System
	{
	public:
		void Update(World& world, float dt) override;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(SpawnerSystem);
	};
}
