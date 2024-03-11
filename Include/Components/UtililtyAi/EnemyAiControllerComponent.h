#pragma once
#include "Systems/System.h"

namespace Engine
{
	class EnemyAiControllerComponent
	{
	public:
		const MetaType* mCurrentState{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(EnemyAiControllerComponent);
	};
}
