#include "Precomp.h"
#include "Utilities/DebugRenderer.h"
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"
#include "Platform/PC/Rendering/DX12Classes/DXPipeline.h"
#include <memory>

class Engine::DebugRenderer::Impl
{
public:
    Impl();
    bool AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color);
    void Render(const glm::mat4& view, const glm::mat4& projection);

private:
    std::vector<std::unique_ptr<DXResource>> lineResources;
};

Engine::DebugRenderer::DebugRenderer()
{

}

Engine::DebugRenderer::~DebugRenderer()
{
}

void Engine::DebugRenderer::AddLine(const glm::vec3&, const glm::vec3&, const glm::vec4&)
{
}

void Engine::DebugRenderer::AddSphere(const glm::vec3&, float, const glm::vec4&)
{
}

void Engine::DebugRenderer::AddCube(const glm::vec3&, float)
{
}

void Engine::DebugRenderer::AddPlane(const glm::vec3&, const glm::vec3&)
{
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
