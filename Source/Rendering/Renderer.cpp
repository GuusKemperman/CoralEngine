#include "Precomp.h"
#include "Rendering/Renderer.h"

#include "Components/CameraComponent.h"
#include "Core/Device.h"
#include "World/World.h"
#include "World/Registry.h"
#include "World/WorldViewport.h"
#include "Rendering/FrameBuffer.h"

#include "Rendering/GPUWorld.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/UIRenderer.h"
#include "Rendering/DebugRenderer.h"
#include "Rendering/PostProcessingRenderer.h"

CE::Renderer::Renderer()
{
	mMeshRenderer = std::make_unique<MeshRenderer>();
	mUIRenderer = std::make_unique<UIRenderer>();
	mDebugRenderer = std::make_unique<DebugRenderer>();
	mPostProcessRenderer = std::make_unique<PostProcessingRenderer>();
}

CE::Renderer::~Renderer() = default;

void CE::Renderer::Render(const World& world)
{
	Render(world, Device::Get().GetDisplaySize());
}

#ifdef EDITOR
void CE::Renderer::RenderToFrameBuffer(
	const World& world, 
	FrameBuffer& buffer, 
	std::optional<glm::vec2> firstResizeBufferTo, 
	bool clearBufferFirst)
{
	mFrameBuffer = &buffer;

	if (firstResizeBufferTo.has_value())
	{
		buffer.Resize(static_cast<glm::ivec2>(*firstResizeBufferTo));
	}

	buffer.Bind();

	if (clearBufferFirst)
	{
		buffer.Clear();
	}

	Render(world, buffer.GetSize());

	buffer.Unbind();
}
#endif // EDITOR

void CE::Renderer::Render(const World& world, glm::vec2 viewportSize)
{
	// Casting const away :(
	WorldViewport& worldViewport = const_cast<WorldViewport&>(world.GetViewport());

	if (CameraComponent::GetSelected(world) == entt::null)
	{
		LOG(LogTemp, Warning, "No camera to render to!");
		return;
	}

	worldViewport.UpdateSize(viewportSize);

	// We run the rendering systems for the world and update the data to be used in the renderers later on.
	world.GetRegistry().RenderSystems();
	world.GetGPUWorld().Update();

	mMeshRenderer->Render(world);
	mPostProcessRenderer->Render(world);
	mDebugRenderer->Render(world);
	mUIRenderer->Render(world);
}
