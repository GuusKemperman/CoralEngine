#include "Precomp.h"
#include "Systems/UpdateTopDownCamSystem.h"

#include "Components/CameraComponent.h"
#include "World/World.h"
#include "World/Registry.h"
#include "World/WorldViewport.h"
#include "Components/TransformComponent.h"
#include "Components/TopDownCamControllerComponent.h"
#include "Core/Input.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Utilities/DrawDebugHelpers.h"

void CE::UpdateTopDownCamSystem::Update(World& world, float dt)
{
	auto& registry = world.GetRegistry();

	const entt::entity activeCameraOwner = CameraComponent::GetSelected(world);
	TopDownCamControllerComponent* const topDownController = registry.TryGet<TopDownCamControllerComponent>(activeCameraOwner);
	TransformComponent* const transform = registry.TryGet<TransformComponent>(activeCameraOwner);

	if (topDownController == nullptr
		|| transform == nullptr
		|| !registry.Valid(topDownController->mTarget))
	{
		return;
	}

	auto target = registry.TryGet<TransformComponent>(topDownController->mTarget);

	if (target == nullptr)
	{
		return;
	}

	float timeScaledZoomDelta{};
	if (topDownController->mUseArrowKeysToEdit)
	{
		const float zoomDelta = Input::Get().GetKeyboardAxis(Input::KeyboardKey::ArrowDown, Input::KeyboardKey::ArrowUp)/* + Input::Get().GetGamepadAxis(0, Input::GamepadAxis::StickLeftY)*/;
		timeScaledZoomDelta = zoomDelta * dt;

		const float rotationInput = Input::Get().GetKeyboardAxis(Input::KeyboardKey::ArrowRight, Input::KeyboardKey::ArrowLeft)/* + Input::Get().GetGamepadAxis(0, Input::GamepadAxis::StickRightX)*/;
		const float timeScaledRotation = rotationInput * dt;

		if (timeScaledRotation != 0)
		{
			topDownController->RotateCameraAroundTarget(timeScaledRotation);
		}
	}
	topDownController->AdjustZoom(timeScaledZoomDelta);

	glm::vec2 cursorDistanceScreenCenter = (world.GetViewport().GetViewportSize() * 0.5f - Input::Get().GetMousePosition()) / world.GetViewport().GetViewportSize();
	cursorDistanceScreenCenter.y *= -1.0f;

	topDownController->ApplyTranslation(*transform, target->GetWorldPosition(), cursorDistanceScreenCenter, dt);
	topDownController->UpdateRotation(*transform, target->GetWorldPosition(), cursorDistanceScreenCenter, dt);
}

void CE::UpdateTopDownCamSystem::Render(const World& world)
{
	auto& registry = world.GetRegistry();

	const auto view = registry.View<TopDownCamControllerComponent, TransformComponent>();

	// Check to see if the view is empty
	if (view.begin() == view.end())
	{
		return;
	}

	for (auto [entity, topdown, transform] : view.each())
	{
		if (!registry.Valid(topdown.mTarget) )
		{
			continue;
		}

		auto target = registry.TryGet<TransformComponent>(topdown.mTarget);

		if (target == nullptr)
		{
			continue;
		}

		DrawDebugBox(world, DebugCategory::Gameplay, topdown.mTargetLocation, glm::vec3(0.1f), glm::vec4(1.0f));
		DrawDebugLine(world, DebugCategory::Gameplay, target->GetWorldPosition(), topdown.mTargetLocation, glm::vec4(1.0f));
	}
}

CE::MetaType CE::UpdateTopDownCamSystem::Reflect()
{
	return MetaType{ MetaType::T<UpdateTopDownCamSystem>{}, "UpdateTopDownCamSystem", MetaType::Base<System>{} };
}
