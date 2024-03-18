#include "Precomp.h"
#include "World/WorldRenderer.h"

#include "Utilities/FrameBuffer.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Components/CameraComponent.h"
#include "Components/TransformComponent.h"
#include "Core/Input.h"
#include "Core/Device.h"
#include "Systems/System.h"
#include "Utilities/DebugRenderer.h"

Engine::WorldRenderer::WorldRenderer(const World& world) :
	mWorld(world),
	mDebugRenderer(std::make_unique<DebugRenderer>()),
	mLastRenderedAtSize(Device::IsHeadless() ? glm::vec2{} : Device::Get().GetDisplaySize())
{
}

Engine::WorldRenderer::~WorldRenderer() = default;

void Engine::WorldRenderer::NewFrame()
{

}

void Engine::WorldRenderer::Render()
{
	RenderAtSize(Device::Get().GetDisplaySize());
}

#ifdef EDITOR
void Engine::WorldRenderer::Render(FrameBuffer& buffer, std::optional<glm::vec2> firstResizeBufferTo, const bool clearBufferFirst)
{
    if (firstResizeBufferTo.has_value())
    {
        buffer.Resize(static_cast<glm::ivec2>(*firstResizeBufferTo));
    }

	buffer.Bind();

	if (clearBufferFirst)
	{
		buffer.Clear();
	}

	RenderAtSize(buffer.GetSize());

	//Framebuffer needs to be bound again before the debug renderer renders
	buffer.Bind();
	mDebugRenderer->Render(mWorld);

	buffer.Unbind();
}
#endif // EDITOR

#ifdef _MSC_VER
#pragma warning(push)
// Disable unreachable code warning, not sure why it's popping 
// up but im sure i'll learn that the hard way in a few days/hours
#pragma warning(disable : 4702)
#endif

std::optional<std::pair<entt::entity, const Engine::CameraComponent&>> Engine::WorldRenderer::GetMainCamera() const
{
	using ReturnPair = std::pair<entt::entity, const CameraComponent&>;
	const Registry& reg = mWorld.get().GetRegistry();

	if (reg.Valid(mMainCamera))
	{
		const CameraComponent* const camera = reg.TryGet<const CameraComponent>(mMainCamera);

		if (camera != nullptr)
		{
			return ReturnPair{ mMainCamera, *camera };
		}
	}

	const auto camerasView = reg.View<const CameraComponent>();

	// MSVC says this is unreachable?

	for (auto [entity, camera] : camerasView.each())
	{
		mMainCamera = entity;
		LOG(LogTemp, Verbose, "Switched to camera {}", static_cast<EntityType>(entity));
		return ReturnPair{ mMainCamera, camera };
	}

	return {};
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

std::optional<std::pair<entt::entity, Engine::CameraComponent&>> Engine::WorldRenderer::GetMainCamera()
{
	const auto constPair = const_cast<const WorldRenderer*>(this)->GetMainCamera();

	if (constPair.has_value())
	{
		return std::pair<entt::entity, CameraComponent&>{ constPair->first, const_cast<CameraComponent&>(constPair->second) };
	}
	return {};
}

glm::vec3 Engine::WorldRenderer::GetScreenToWorldDirection(glm::vec2 screenPosition) const
{
	const auto camera = GetMainCamera();

    if (!camera.has_value())
    {
        return { 1.0f, 0.0f, 0.0f };
    }

	screenPosition -= mLastRenderedAtPos;
	const glm::mat4& invMat = camera->second.mInvViewProjection;
	const glm::vec4 nearVec = glm::vec4((screenPosition.x - (mLastRenderedAtSize.x * .5f)) / (mLastRenderedAtSize.x * .5f), -1 * (screenPosition.y - (mLastRenderedAtSize.y * .5f)) / (mLastRenderedAtSize.y * .5f), -1, 1.0);
	const glm::vec4 farVec = glm::vec4((screenPosition.x - (mLastRenderedAtSize.x * .5f)) / (mLastRenderedAtSize.x * .5f), -1 * (screenPosition.y - (mLastRenderedAtSize.y * .5f)) / (mLastRenderedAtSize.y * .5f), 1, 1.0);
	glm::vec4 nearResult = invMat * nearVec;
	glm::vec4 farResult = invMat * farVec;
	nearResult /= nearResult.w;
	farResult /= farResult.w;
	const glm::vec3 dir = glm::normalize(glm::vec3(farResult) - glm::vec3(nearResult));

	return normalize(dir);
}

glm::vec3 Engine::WorldRenderer::ScreenToWorld(glm::vec2 screenPosition, float distanceFromCamera) const
{
	const auto camera = GetMainCamera();
	if (!camera.has_value())
	{
		return {};
	}

	glm::vec3 dir = GetScreenToWorldDirection(screenPosition);

	if (glm::isnan(dir.x)
		|| glm::isnan(dir.y)
		|| glm::isnan(dir.z))
	{
		return {};
	}

	const TransformComponent* transform = mWorld.get().GetRegistry().TryGet<TransformComponent>(camera->first);
	const glm::vec3 camPosition = transform == nullptr ? glm::vec3{} : transform->GetWorldPosition();

	return camPosition + dir * distanceFromCamera;

}

void Engine::WorldRenderer::RenderAtSize(glm::vec2 size)
{
	mLastRenderedAtSize = size;

	// If we are not using the editor, we always
	// render starting from the topleft corner.
#ifdef EDITOR
	mLastRenderedAtPos = ImGui::GetCursorScreenPos();
#endif // EDITOR

	GetWorld().GetRegistry().RenderSystems();
}
