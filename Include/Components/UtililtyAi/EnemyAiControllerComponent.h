#pragma once
#include "Systems/System.h"

namespace Engine
{
	class EnemyAiControllerComponent
	{
	public:
		EnemyAiControllerComponent();

		void UpdateState(World& world, entt::entity enemyID);

		int CurrentState = 0;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(EnemyAiControllerComponent);
	};
}
