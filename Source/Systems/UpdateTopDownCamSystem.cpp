#include "Precomp.h"
#include "Systems/UpdateTopDownCamSystem.h"

#include "World/World.h"
#include "World/WorldRenderer.h"
#include "World/Registry.h"
#include "Components/TransformComponent.h"
#include "Components/TopDownCamControllerComponent.h"
#include "Core/InputManager.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

void Engine::UpdateTopDownCamSystem::Update(World& world, float dt)
{
	dt;

	auto& registry = world.GetRegistry();

	const auto view = registry.View<TopDownCamControllerComponent, TransformComponent>();

	// Check to see if the view is empty
	if (view.begin() == view.end())
	{
		return;
	}

	const glm::vec2 cursorDistanceScreenCenter = InputManager::GetMousePos() - world.GetRenderer().GetViewportSize();

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

		topdown.ApplyTranslation(transform, cursorDistanceScreenCenter);
		topdown.UpdateRotation(transform, target->GetWorldPosition());
	}
}

Engine::MetaType Engine::UpdateTopDownCamSystem::Reflect()
{
	return MetaType{ MetaType::T<UpdateTopDownCamSystem>{}, "UpdateTopDownCamSystem", MetaType::Base<System>{} };
}
