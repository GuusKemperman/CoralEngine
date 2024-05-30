#include "Precomp.h"
#include "Systems/UISystem.h"

#include "World/Registry.h"
#include "Components/UI/UIButtonComponent.h"
#include "Components/TransformComponent.h"
#include "Core/Input.h"

void CE::UISystem::Update(World& world, float dt)
{
	mSecondsSinceLastNavigationChange += dt;

	Registry& reg = world.GetRegistry();

	entt::entity selectedEntity = reg.View<UIButtonTag, UIButtonSelectedTag>().front();

	if (selectedEntity == entt::null)
	{
		selectedEntity = reg.View<UIButtonTag>().front();

		if (selectedEntity != entt::null)
		{
			UIButtonTag::Select(world, selectedEntity);
		}
		else
		{
			return;
		}
	}

	selectedEntity = CheckNavigation(world,
		selectedEntity,
		UISystem::Edges::Top,
		Input::KeyboardKey::ArrowUp,
		Input::KeyboardKey::W,
		0,
		Input::GamepadButton::DPadUp,
		Input::GamepadAxis::StickLeftY,
		Input::GamepadAxis::StickRightY,
		false);

	selectedEntity = CheckNavigation(world,
		selectedEntity,
		UISystem::Edges::Bottom,
		Input::KeyboardKey::ArrowDown,
		Input::KeyboardKey::S,
		0,
		Input::GamepadButton::DPadDown,
		Input::GamepadAxis::StickLeftY,
		Input::GamepadAxis::StickRightY,
		true);

	selectedEntity = CheckNavigation(world,
		selectedEntity,
		UISystem::Edges::Left,
		Input::KeyboardKey::ArrowLeft,
		Input::KeyboardKey::A,
		0,
		Input::GamepadButton::DPadLeft,
		Input::GamepadAxis::StickLeftX,
		Input::GamepadAxis::StickRightX,
		true);

	selectedEntity = CheckNavigation(world,
		selectedEntity,
		UISystem::Edges::Right,
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

	for (const BoundEvent& boundEvent : mOnClickEvents)
	{
		entt::sparse_set* storage = reg.Storage(boundEvent.mType.get().GetTypeId());

		if (storage == nullptr
			|| !storage->contains(selectedEntity))
		{
			continue;
		}

		if (boundEvent.mIsStatic)
		{
			boundEvent.mFunc.get().InvokeUncheckedUnpacked(world, selectedEntity);
		}
		else
		{
			MetaAny component{ boundEvent.mType.get(), storage->value(selectedEntity), false };
			boundEvent.mFunc.get().InvokeUncheckedUnpacked(component, world, selectedEntity);
		}
	}
}

entt::entity CE::UISystem::CheckNavigation(World& world, 
	entt::entity currentEntity,
	UISystem::Edges edge,
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
		const bool hasButton = world.GetRegistry().HasComponent<UIButtonTag>(currentEntity);
		const TransformComponent* const transform = world.GetRegistry().TryGet<TransformComponent>(currentEntity);

		if (!hasButton 
			|| transform == nullptr)
		{
			return currentEntity;
		}

		entt::entity entityToNavigateTo = entt::null;
		glm::vec2 pos = { transform->GetWorldPosition().x, transform->GetWorldPosition().y };
		float lowestDistance = std::numeric_limits<float>::infinity();

		auto isLocatedInDirection = [&](const glm::vec2 pos2) -> bool 
			{
				switch (edge)
				{
				case UISystem::Edges::Top:
					return pos2.y > pos.y ? true : false;
				case UISystem::Edges::Right:
					return pos2.x > pos.x ? true : false;
				case UISystem::Edges::Bottom:
					return pos2.y < pos.y ? true : false;
				case UISystem::Edges::Left:
					return pos2.x < pos.x ? true : false;
				default:
					return false;
				}
			};

		for (auto [entity, UITransform] : world.GetRegistry().View<UIButtonTag, TransformComponent>().each())
		{
			if (entity == currentEntity)
			{
				continue;
			}

			const glm::vec3 worldPosition = UITransform.GetWorldPosition();

			const glm::vec2 UIPosition = { worldPosition.x, worldPosition.y };

			float distance = glm::distance(pos, UIPosition); 

			if (isLocatedInDirection(UIPosition) 
				&& distance < lowestDistance)
			{
				entityToNavigateTo = entity;
				lowestDistance = distance;
			}
		}

		if (entityToNavigateTo != entt::null)
		{
			UIButtonTag::Select(world, entityToNavigateTo);
			currentEntity = entityToNavigateTo;
			mSecondsSinceLastNavigationChange = 0.0f;
		}
	}

	return currentEntity;
}

CE::MetaType CE::UISystem::Reflect()
{
	return MetaType{ MetaType::T<UISystem>{}, "UISystem", MetaType::Base<System>{} };
}
