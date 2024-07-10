#include "Precomp.h"
#include "Systems/UpdateFlyCamSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/TransformComponent.h"
#include "Components/FlyCamControllerComponent.h"
#include "Core/InputManager.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

void Engine::UpdateFlyCamSystem::Update(World& world, float dt)
{
	const float mouseWheelChange = InputManager::GetScrollY();
	const bool movementSpeedMultiply = mouseWheelChange >= 0.0f;

	const glm::vec3 movementInput
	{
		 InputManager::GetAxis(ImGuiKey_W, ImGuiKey_S),
		 InputManager::GetAxis(ImGuiKey_A, ImGuiKey_D),
		 InputManager::GetAxis(ImGuiKey_Space, ImGuiKey_LeftCtrl)
	};

	const glm::vec3 timeScaledMovementInput = movementInput * dt;

	constexpr Axis rotateAround[2]
	{
		Axis::Right,
		Axis::Up
	};

	const glm::vec2 rotationInput
	{
		InputManager::GetAxis(ImGuiKey_DownArrow, ImGuiKey_UpArrow),
		InputManager::GetAxis(ImGuiKey_LeftArrow, ImGuiKey_RightArrow)
	};

	const glm::vec2 timeScaledRotationInput = rotationInput * dt;

	const std::array<glm::quat, 2> timeScaledRotations
	{
		ToVector3(rotateAround[0]) * timeScaledRotationInput[0],
		ToVector3(rotateAround[1]) * timeScaledRotationInput[1],
	};

	const bool emptyMovement = timeScaledMovementInput == glm::vec3{};
	const bool emptyRotation = timeScaledRotations[0] == glm::identity<glm::quat>() && timeScaledRotations[1] == glm::identity<glm::quat>();
	const bool emptyMouseWheel = mouseWheelChange == 0;

	if (emptyMovement
		&& emptyRotation
		&& emptyMouseWheel)
	{
		return;
	}

	const auto view = world.GetRegistry().View<FlyCamControllerComponent, TransformComponent>();

	for (auto [entity, flycam, transform] : view.each())
	{
		if (!emptyMouseWheel)
		{
			flycam.AdjustMovementSpeed(movementSpeedMultiply);
		}

		if (!emptyMovement)
		{
			flycam.ApplyTranslation(transform, timeScaledMovementInput);
		}

		if (!emptyRotation)
		{
			flycam.ApplyRotation(transform, timeScaledRotations);
		}
	}
}

Engine::MetaType Engine::UpdateFlyCamSystem::Reflect()
{
	return MetaType{ MetaType::T<UpdateFlyCamSystem>{}, "UpdateFlyCamSystem", MetaType::Base<System>{} };
}
