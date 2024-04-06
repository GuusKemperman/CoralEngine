#include "Precomp.h"
#include "Systems/UpdateFlyCamSystem.h"

#include "Components/CameraComponent.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Components/TransformComponent.h"
#include "Components/FlyCamControllerComponent.h"
#include "Core/Input.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "World/WorldViewport.h"

void CE::UpdateFlyCamSystem::Update(World& world, float dt)
{
	const entt::entity activeCameraOwner = CameraComponent::GetSelected(world);

	FlyCamControllerComponent* const flyCam = world.GetRegistry().TryGet<FlyCamControllerComponent>(activeCameraOwner);
	TransformComponent* const transform = world.GetRegistry().TryGet<TransformComponent>(activeCameraOwner);

	if (flyCam == nullptr
		|| transform == nullptr)
	{
		return;
	}

	glm::vec3 movementInput{};

	Input& input = Input::Get();

	movementInput[Axis::Forward] = input.GetKeyboardAxis(Input::KeyboardKey::W, Input::KeyboardKey::S) * input.IsKeyboardKeyHeld(Input::KeyboardKey::LeftAlt) + -input.GetGamepadAxis(0, Input::GamepadAxis::StickLeftY);
	movementInput[Axis::Up] = input.GetKeyboardAxis(Input::KeyboardKey::E, Input::KeyboardKey::Q) * input.IsKeyboardKeyHeld(Input::KeyboardKey::LeftAlt);
	movementInput[Axis::Right] =  input.GetKeyboardAxis(Input::KeyboardKey::D, Input::KeyboardKey::A) * input.IsKeyboardKeyHeld(Input::KeyboardKey::LeftAlt) + input.GetGamepadAxis(0, Input::GamepadAxis::StickLeftX);

	const glm::vec3 timeScaledMovementInput = movementInput * dt;

	constexpr Axis::Values rotateAround[2]
	{
		Axis::Right,
		Axis::Up
	};

	const glm::vec2 mouseInput = input.GetDeltaMousePosition() / world.GetViewport().GetViewportSize() * 100.0f * input.IsMouseButtonHeld(Input::MouseButton::Right, true);

	const glm::vec2 rotationInput
	{
		mouseInput.y + input.GetKeyboardAxis(Input::KeyboardKey::ArrowDown, Input::KeyboardKey::ArrowUp) + input.GetGamepadAxis(0, Input::GamepadAxis::StickRightY),
		mouseInput.x + input.GetKeyboardAxis(Input::KeyboardKey::ArrowRight, Input::KeyboardKey::ArrowLeft) + input.GetGamepadAxis(0, Input::GamepadAxis::StickRightX)
	};

	const glm::vec2 timeScaledRotationInput = rotationInput * dt;

	const std::array<glm::quat, 2> timeScaledRotations
	{
		ToVector3(rotateAround[0]) * timeScaledRotationInput[0],
		ToVector3(rotateAround[1]) * timeScaledRotationInput[1],
	};

	const bool emptyMovement = timeScaledMovementInput == glm::vec3{};
	const bool emptyRotation = timeScaledRotations[0] == glm::identity<glm::quat>() && timeScaledRotations[1] == glm::identity<glm::quat>();

	if (!emptyMovement)
	{
		flyCam->ApplyTranslation(*transform, timeScaledMovementInput);
	}

	if (!emptyRotation)
	{
		flyCam->ApplyRotation(*transform, timeScaledRotations);
	}
}

CE::MetaType CE::UpdateFlyCamSystem::Reflect()
{
	return MetaType{ MetaType::T<UpdateFlyCamSystem>{}, "UpdateFlyCamSystem", MetaType::Base<System>{} };
}
