#pragma once
#include "Systems/System.h"
#include "Core/Input.h"

namespace CE
{
	class UIButtonTag;

	class UISystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mShouldTickWhilstPaused = true;
			return traits;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UISystem);

		enum Edges
		{
			Top,
			Right,
			Bottom,
			Left
		};

		entt::entity CheckNavigation(World& world,
			entt::entity currentEntity,
			UISystem::Edges edge,
			Input::KeyboardKey key1, 
			Input::KeyboardKey key2, 
			int gamePadId, 
			Input::GamepadButton gamepadButton, 
			Input::GamepadAxis axis1, 
			Input::GamepadAxis axis2,
			bool negateAxis);

		static constexpr float sJoyStickNavigationCooldown = 0.1f;
		static constexpr float sJoyStickMinMovementToNavigate = 0.1f;
		float mSecondsSinceLastNavigationChange{};
	};
}