#include "Precomp.h"
#include "Systems/UISystem.h"

#include "World/Registry.h"
#include "Components/UI/UIButtonComponent.h"
#include "Core/Input.h"

void CE::UISystem::Update(World& world, float dt)
{
	mSecondsSinceLastNavigationChange += dt;

	Registry& reg = world.GetRegistry();

	entt::entity selectedEntity = reg.View<UIButtonComponent, UIButtonSelectedTag>().front();

	if (selectedEntity == entt::null)
	{
		selectedEntity = reg.View<UIButtonComponent>().front();

		if (selectedEntity != entt::null)
		{
			UIButtonComponent::Select(world, selectedEntity);
		}
		else
		{
			return;
		}
	}

	selectedEntity = CheckNavigation(world, 
		selectedEntity, 
		&UIButtonComponent::mButtonTopSide, 
		Input::KeyboardKey::ArrowUp, 
		Input::KeyboardKey::W, 
		0, 
		Input::GamepadButton::DPadUp, 
		Input::GamepadAxis::StickLeftY, 
		Input::GamepadAxis::StickRightY,
		false);

	selectedEntity = CheckNavigation(world,
		selectedEntity,
		&UIButtonComponent::mButtonBottomSide,
		Input::KeyboardKey::ArrowDown,
		Input::KeyboardKey::S,
		0,
		Input::GamepadButton::DPadDown,
		Input::GamepadAxis::StickLeftY,
		Input::GamepadAxis::StickRightY,
		true);

	selectedEntity = CheckNavigation(world,
		selectedEntity,
		&UIButtonComponent::mButtonLeftSide,
		Input::KeyboardKey::ArrowLeft,
		Input::KeyboardKey::A,
		0,
		Input::GamepadButton::DPadLeft,
		Input::GamepadAxis::StickLeftX,
		Input::GamepadAxis::StickRightX,
		true);

	selectedEntity = CheckNavigation(world,
		selectedEntity,
		&UIButtonComponent::mButtonRightSide,
		Input::KeyboardKey::ArrowRight,
		Input::KeyboardKey::D,
		0,
		Input::GamepadButton::DPadRight,
		Input::GamepadAxis::StickLeftX,
		Input::GamepadAxis::StickRightX,
		false);


	Input& input = Input::Get();


	if (selectedEntity == entt::null
		|| (!input.WasKeyboardKeyPressed(Input::KeyboardKey::Enter)
			&& !input.WasKeyboardKeyPressed(Input::KeyboardKey::NumpadEnter)
			&& !input.WasGamepadButtonPressed(0, Input::GamepadButton::South)))
	{
		return;
	}


	for (auto&& [typeId, storage] : reg.Storage())
	{
		if (!storage.contains(selectedEntity))
		{
			continue;
		}

		const MetaType* const metaType = MetaManager::Get().TryGetType(typeId);

		if (metaType == nullptr)
		{
			continue;
		}

		const MetaFunc* const pressedEvent = TryGetEvent(*metaType, sButtonPressEvent);

		if (pressedEvent == nullptr)
		{
			continue;
		}

		if (pressedEvent->GetProperties().Has(Props::sIsEventStaticTag))
		{
			pressedEvent->InvokeUncheckedUnpacked(world, selectedEntity);
		}
		else
		{
			MetaAny component{ *metaType, storage.value(selectedEntity), false };
			pressedEvent->InvokeUncheckedUnpacked(component, world, selectedEntity);
		}
	}

}

entt::entity CE::UISystem::CheckNavigation(World& world, 
	entt::entity currentEntity,
	entt::entity UIButtonComponent::* ptrToField,
	Input::KeyboardKey key1, 
	Input::KeyboardKey key2, 
	int gamePadId,
	Input::GamepadButton gamepadButton, 
	Input::GamepadAxis axis1, 
	Input::GamepadAxis axis2,
	bool negateAxis)
{
	Input& input = Input::Get();

	auto isAxisActive = [&](Input::GamepadAxis axis)
		{
			if (mSecondsSinceLastNavigationChange < sJoyStickNavigationCooldown)
			{
				return false;
			}

			const float value = input.GetGamepadAxis(gamePadId, axis);

			return negateAxis ? value <= -sJoyStickMinMovementToNavigate : value >= sJoyStickMinMovementToNavigate;;
		};

	if (input.WasKeyboardKeyPressed(key1)
		|| input.WasKeyboardKeyPressed(key2)
		|| input.WasGamepadButtonPressed(gamePadId, gamepadButton)
		|| isAxisActive(axis1)
		|| isAxisActive(axis2))
	{
		const UIButtonComponent* const button = world.GetRegistry().TryGet<UIButtonComponent>(currentEntity);

		if (button != nullptr)
		{
			const entt::entity entityToNavigateTo = button->*ptrToField;

			if (entityToNavigateTo != entt::null)
			{
				UIButtonComponent::Select(world, entityToNavigateTo);
				currentEntity = entityToNavigateTo;
				mSecondsSinceLastNavigationChange = 0.0f;
			}
		}
	}

	return currentEntity;
}

CE::MetaType CE::UISystem::Reflect()
{
	return MetaType{ MetaType::T<UISystem>{}, "UISystem", MetaType::Base<System>{} };
}
