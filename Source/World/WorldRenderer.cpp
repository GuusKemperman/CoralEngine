#include "Precomp.h"
#include "World/WorldRenderer.h"

#include "Utilities/FrameBuffer.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Components/CameraComponent.h"
#include "xsr.hpp"
#include "Components/TransformComponent.h"
#include "Core/Input.h"
#include "Core/Renderer.h"

Engine::WorldRenderer::WorldRenderer(const World& world) :
	mWorld(world),
	mLastRenderedAtSize(Renderer::Get().GetDisplaySize())
{
}

Engine::WorldRenderer::~WorldRenderer() = default;

void Engine::WorldRenderer::NewFrame()
{

}

void Engine::WorldRenderer::Render()
{
	RenderAtSize(Renderer::Get().GetDisplaySize());
}

#ifdef EDITOR
void Engine::WorldRenderer::Render(FrameBuffer& buffer, std::optional<glm::vec2> firstResizeBufferTo, const bool clearBufferFirst)
{
	buffer.Bind();

	if (clearBufferFirst)
	{
		buffer.Clear();
	}

	if (firstResizeBufferTo.has_value())
	{
		buffer.Resize(static_cast<glm::ivec2>(*firstResizeBufferTo));
	}

	RenderAtSize(buffer.GetSize());

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

void Engine::WorldRenderer::AddLine(DebugCategory::Enum category, const glm::vec3 from, const glm::vec3 to, const glm::vec4 color) const
{
    if (!(sDebugCategoryFlags & category)) return;

    xsr::render_debug_line(value_ptr(from), value_ptr(to), value_ptr(color));
}

void Engine::WorldRenderer::AddLine(DebugCategory::Enum category, const glm::vec2 from, const glm::vec2 to, const glm::vec4 color, Plane::Enum plane) const
{
    if (!(sDebugCategoryFlags & category)) return;

    switch (plane)
    {
    case Plane::XY:
        AddLine(category, glm::vec3(from.x, from.y, 0.0f), glm::vec3(to.x, to.y, 0.0f), color);
        break;
    case Plane::XZ:
        AddLine(category, glm::vec3(from.x, 0.0f, from.y), glm::vec3(to.x, 0.0f, to.y), color);
        break;
    case Plane::YZ:
        AddLine(category, glm::vec3(0.0f, from.x, from.y), glm::vec3(0.0f, to.x, to.y), color);
        break;
    }
}

void Engine::WorldRenderer::AddCircle(DebugCategory::Enum category, const glm::vec3 center, float radius, const glm::vec4 color, Plane::Enum plane) const
{
    if (!(sDebugCategoryFlags & category)) return;

    constexpr float dt = glm::two_pi<float>() / 64.0f;
    float t = 0.0f;

    glm::vec3 v0;

    switch (plane)
    {
    case Plane::XY:
        v0 = { center.x + radius * cos(t), center.y + radius * sin(t), center.z };
        for (; t < glm::two_pi<float>() - dt; t += dt)
        {
	        glm::vec3 v1(center.x + radius * cos(t + dt), center.y + radius * sin(t + dt), center.z);
            AddLine(category, v0, v1, color);
            v0 = v1;
        }
        break;
    case Plane::XZ:
        v0 = { center.x + radius * cos(t), center.y, center.z + radius * sin(t) };
        for (; t < glm::two_pi<float>() - dt; t += dt)
        {
	        glm::vec3 v1(center.x + radius * cos(t + dt), center.y, center.z + radius * sin(t + dt));
            AddLine(category, v0, v1, color);
            v0 = v1;
        }
        break;
    case Plane::YZ:
        v0 = { center.x, center.y + radius * cos(t), center.z + radius * sin(t) };
        for (; t < glm::two_pi<float>() - dt; t += dt)
        {
	        glm::vec3 v1(center.x, center.y + radius * cos(t + dt), center.z + radius * sin(t + dt));
            AddLine(category, v0, v1, color);
            v0 = v1;
        }
        break;
    }
}

void Engine::WorldRenderer::AddSphere(DebugCategory::Enum category, const glm::vec3 center, float radius, const glm::vec4 color) const
{
    if (!(sDebugCategoryFlags & category)) return;

    constexpr float dt = glm::two_pi<float>() / 64.0f;
    float t = 0.0f;
    glm::vec3 v0;

    // xy
    v0 = { center.x + radius * cos(t), center.y + radius * sin(t), center.z };
    for (; t < glm::two_pi<float>() - dt; t += dt)
    {
	    glm::vec3 v1(center.x + radius * cos(t + dt), center.y + radius * sin(t + dt), center.z);
        AddLine(category, v0, v1, color);
        v0 = v1;
    }
    // xz
    t = 0.0f;
    v0 = { center.x + radius * cos(t), center.y, center.z + radius * sin(t) };
    for (; t < glm::two_pi<float>() - dt; t += dt)
    {
	    glm::vec3 v1(center.x + radius * cos(t + dt), center.y, center.z + radius * sin(t + dt));
        AddLine(category, v0, v1, color);
        v0 = v1;
    }
    // yz
    t = 0.0f;
    v0 = { center.x, center.y + radius * cos(t), center.z + radius * sin(t) };
    for (; t < glm::two_pi<float>() - dt; t += dt)
    {
	    glm::vec3 v1(center.x, center.y + radius * cos(t + dt), center.z + radius * sin(t + dt));
        AddLine(category, v0, v1, color);
        v0 = v1;
    }
}

void Engine::WorldRenderer::AddSquare(DebugCategory::Enum category, const glm::vec3 center, float size, const glm::vec4 color, Plane::Enum plane) const
{
    if (!(sDebugCategoryFlags & category)) return;

    const float s = size * 0.5f;
    glm::vec3 A{}, B{}, C{}, D{};

    switch (plane)
    {
    case Plane::XY:
        A = center + glm::vec3(-s, -s, 0.f);
        B = center + glm::vec3(-s, s, 0.f);
        C = center + glm::vec3(s, s, 0.f);
        D = center + glm::vec3(s, -s, 0.f);
        break;
    case Plane::XZ:
        A = center + glm::vec3(-s, 0.f, -s);
        B = center + glm::vec3(-s, 0.f, s);
        C = center + glm::vec3(s, 0.f, s);
        D = center + glm::vec3(s, 0.f, -s);
        break;
    case Plane::YZ:
        A = center + glm::vec3(0.f, -s, -s);
        B = center + glm::vec3(0.f, -s, s);
        C = center + glm::vec3(0.f, s, s);
        D = center + glm::vec3(0.f, s, -s);
        break;
    }

    AddLine(category, A, B, color);
    AddLine(category, B, C, color);
    AddLine(category, C, D, color);
    AddLine(category, D, A, color);
}

void Engine::WorldRenderer::AddBox(DebugCategory::Enum category, const glm::vec3 center, const glm::vec3 halfExtents, const glm::vec4 color) const
{
    if (!(sDebugCategoryFlags & category)) return;

    // Generated using chatgpt

    // Calculate the minimum and maximum corner points of the box
    const glm::vec3 minCorner = center - halfExtents;
    const glm::vec3 maxCorner = center + halfExtents;

    // Generate the lines for the box
    // Front face
    AddLine(category, glm::vec3(minCorner.x, minCorner.y, minCorner.z), glm::vec3(maxCorner.x, minCorner.y, minCorner.z), color);
    AddLine(category, glm::vec3(maxCorner.x, minCorner.y, minCorner.z), glm::vec3(maxCorner.x, maxCorner.y, minCorner.z), color);
    AddLine(category, glm::vec3(maxCorner.x, maxCorner.y, minCorner.z), glm::vec3(minCorner.x, maxCorner.y, minCorner.z), color);
    AddLine(category, glm::vec3(minCorner.x, maxCorner.y, minCorner.z), glm::vec3(minCorner.x, minCorner.y, minCorner.z), color);

    // Back face
    AddLine(category, glm::vec3(minCorner.x, minCorner.y, maxCorner.z), glm::vec3(maxCorner.x, minCorner.y, maxCorner.z), color);
    AddLine(category, glm::vec3(maxCorner.x, minCorner.y, maxCorner.z), glm::vec3(maxCorner.x, maxCorner.y, maxCorner.z), color);
    AddLine(category, glm::vec3(maxCorner.x, maxCorner.y, maxCorner.z), glm::vec3(minCorner.x, maxCorner.y, maxCorner.z), color);
    AddLine(category, glm::vec3(minCorner.x, maxCorner.y, maxCorner.z), glm::vec3(minCorner.x, minCorner.y, maxCorner.z), color);

    // Connecting lines between the front and back faces
    AddLine(category, glm::vec3(minCorner.x, minCorner.y, minCorner.z), glm::vec3(minCorner.x, minCorner.y, maxCorner.z), color);
    AddLine(category, glm::vec3(maxCorner.x, minCorner.y, minCorner.z), glm::vec3(maxCorner.x, minCorner.y, maxCorner.z), color);
    AddLine(category, glm::vec3(maxCorner.x, maxCorner.y, minCorner.z), glm::vec3(maxCorner.x, maxCorner.y, maxCorner.z), color);
    AddLine(category, glm::vec3(minCorner.x, maxCorner.y, minCorner.z), glm::vec3(minCorner.x, maxCorner.y, maxCorner.z), color);
}

void Engine::WorldRenderer::AddPolygon(DebugCategory::Enum category, std::vector<glm::vec3>& points, glm::vec4 color) const
{
    size_t pointCount = points.size();
    for (size_t i = 0; i < pointCount; i++)
    {
        AddLine(category, points[i], points[(i + 1) % pointCount], color);
    }
}
