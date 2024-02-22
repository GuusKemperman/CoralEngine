#include "Precomp.h"
#include "Systems/UpdateTopDownCamSystem.h"

#include "World/World.h"
#include "World/WorldRenderer.h"
#include "World/Registry.h"
#include "Components/TransformComponent.h"
#include "Components/TopDownCamControllerComponent.h"
#include "Core/Input.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

void Engine::UpdateTopDownCamSystem::Update(World& world, float dt)
{
	auto& registry = world.GetRegistry();

	const auto view = registry.View<TopDownCamControllerComponent, TransformComponent>();

	// Check to see if the view is empty
	if (view.begin() == view.end())
	{
		return;
	}

	const float zoomDelta = Input::Get().GetKeyboardAxis(Input::KeyboardKey::ArrowDown, Input::KeyboardKey::ArrowUp) + Input::Get().GetGamepadAxis(0, Input::GamepadAxis::StickLeftY);
	const float timeScaledZoomDelta = zoomDelta * dt;

	const float rotationInput = Input::Get().GetKeyboardAxis(Input::KeyboardKey::ArrowLeft, Input::KeyboardKey::ArrowRight) + Input::Get().GetGamepadAxis(0, Input::GamepadAxis::StickRightX);
	const float timeScaledRotation = rotationInput * dt;

	const glm::vec2 cursorDistanceScreenCenter = world.GetRenderer().GetViewportSize() * 0.5f - Input::Get().GetMousePosition();

	const bool emptyRotation = timeScaledRotation == 0;
	const bool emptyZoom = timeScaledZoomDelta == 0;

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

		if (!emptyZoom)
		{
			topdown.AdjustZoom(timeScaledZoomDelta);
		}

		if (!emptyRotation)
		{
			topdown.RotateCameraAroundTarget(timeScaledRotation);
		}

		topdown.ApplyTranslation(transform, target->GetWorldPosition(), cursorDistanceScreenCenter);
		topdown.UpdateRotation(transform, target->GetWorldPosition());
	}
}

Engine::MetaType Engine::UpdateTopDownCamSystem::Reflect()
{
	return MetaType{ MetaType::T<UpdateTopDownCamSystem>{}, "UpdateTopDownCamSystem", MetaType::Base<System>{} };
}
