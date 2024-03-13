#include "Precomp.h"
#include "Systems/UpdateFlyCamSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/TransformComponent.h"
#include "Components/FlyCamControllerComponent.h"
#include "Core/Input.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "World/WorldRenderer.h"

void Engine::UpdateFlyCamSystem::Update(World& world, float dt)
{
	auto activeCamera = world.GetRenderer().GetMainCamera();
	const entt::entity activeCameraOwner = activeCamera.has_value() ? activeCamera->first : entt::null;

	FlyCamControllerComponent* const flyCam = world.GetRegistry().TryGet<FlyCamControllerComponent>(activeCameraOwner);
	TransformComponent* const transform = world.GetRegistry().TryGet<TransformComponent>(activeCameraOwner);

	if (flyCam == nullptr
		|| transform == nullptr)
	{
		return;
	}

	glm::vec3 movementInput{};

	movementInput[Axis::Forward] = Input::Get().GetKeyboardAxis(Input::KeyboardKey::W, Input::KeyboardKey::S);
	movementInput[Axis::Up] = Input::Get().GetKeyboardAxis(Input::KeyboardKey::E, Input::KeyboardKey::Q);
	movementInput[Axis::Right] =  Input::Get().GetKeyboardAxis(Input::KeyboardKey::D, Input::KeyboardKey::A);

	const glm::vec3 timeScaledMovementInput = movementInput * dt;

	constexpr Axis::Values rotateAround[2]
	{
		Axis::Right,
		Axis::Up
	};

	const glm::vec2 rotationInput
	{
		Input::Get().GetKeyboardAxis(Input::KeyboardKey::ArrowDown, Input::KeyboardKey::ArrowUp),
		Input::Get().GetKeyboardAxis(Input::KeyboardKey::ArrowRight, Input::KeyboardKey::ArrowLeft)
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

Engine::MetaType Engine::UpdateFlyCamSystem::Reflect()
{
	return MetaType{ MetaType::T<UpdateFlyCamSystem>{}, "UpdateFlyCamSystem", MetaType::Base<System>{} };
}
