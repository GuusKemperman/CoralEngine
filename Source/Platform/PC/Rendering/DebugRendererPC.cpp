#include "Precomp.h"
#include "Utilities/DebugRenderer.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"

class Engine::DebugRenderer::Impl
{
public:
    Impl();
    bool AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color);
    void Render(const glm::mat4& view, const glm::mat4& projection);
};

Engine::DebugRenderer::DebugRenderer()
{

}

Engine::DebugRenderer::~DebugRenderer()
{
}

void Engine::DebugRenderer::AddLine(DebugCategory::Enum category, const glm::vec3& from, const glm::vec3& to, const glm::vec4& color) const
{
    if (!(sDebugCategoryFlags & category)) return;
    mImpl->AddLine(from, to, color);
}

void Engine::DebugRenderer::Render(const World& world)
{
    const auto cameraView = world.GetRegistry().View<const TransformComponent, const CameraComponent>();

    for (auto [entity, cameraTransform, camera] : cameraView.each())
    {
        mImpl->Render(camera.GetView(), camera.GetProjection());
    }
}

Engine::DebugRenderer::Impl::Impl()
{
}

bool Engine::DebugRenderer::Impl::AddLine(const glm::vec3&, const glm::vec3&, const glm::vec4&)
{
    return false;
}

void Engine::DebugRenderer::Impl::Render(const glm::mat4&, const glm::mat4&)
{
}
