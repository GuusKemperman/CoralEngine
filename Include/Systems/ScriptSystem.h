#pragma once
#include "Systems/System.h"

namespace Engine
{
	class ScriptSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ScriptSystem);
	};
}