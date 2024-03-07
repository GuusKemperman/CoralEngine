#pragma once
#include "Systems/System.h"

namespace Engine
{
	class CharacterComponent;
	struct AbilityInstanceWithInputs;

	class AbilitySystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

	private:
		void ActivateAbility(World& world, entt::entity castBy, CharacterComponent& characterData, AbilityInstanceWithInputs& ability);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilitySystem);
	};
}
