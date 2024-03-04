#include "Precomp.h"
#include "Systems/RenderToCamerasSystem.h"

#include <glm/gtc/type_ptr.hpp>

#include "World/World.h"
#include "World/WorldRenderer.h"
#include "Components/CameraComponent.h"
#include "xsr.hpp"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

void Engine::RenderToCamerasSystem::Render(const World& world)
{
	xsr::render_directional_light(value_ptr(normalize(glm::vec3{ .5f, -1.5f, .5f })), value_ptr(glm::vec3{ 1.0f }));

	const auto optionalEntityCameraPair = world.GetRenderer().GetMainCamera();
	ASSERT_LOG(optionalEntityCameraPair.has_value(), "XSR draw requests have been made, but they cannot be cleared as there is no camera to draw them to");

	const auto camera = optionalEntityCameraPair->second;

	xsr::render(value_ptr(camera.GetView()), value_ptr(camera.GetProjection()));
}

Engine::MetaType Engine::RenderToCamerasSystem::Reflect()
{
	return MetaType{ MetaType::T<RenderToCamerasSystem>{}, "RenderToCamerasSystem", MetaType::Base<System>{} };
}
