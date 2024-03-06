#include "Precomp.h"
#include "Utilities/DebugRenderer.h"

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

void Engine::DebugRenderer::AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color)
{
}

void Engine::DebugRenderer::AddSphere(const glm::vec3& center, float radius, const glm::vec4& color)
{
}

void Engine::DebugRenderer::AddCube(const glm::vec3& center, float size)
{
}

void Engine::DebugRenderer::AddPlane(const glm::vec3& p1, const glm::vec3& p2)
{
}

Engine::DebugRenderer::Impl::Impl()
{
}

bool Engine::DebugRenderer::Impl::AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color)
{
    return false;
}

void Engine::DebugRenderer::Impl::Render(const glm::mat4& view, const glm::mat4& projection)
{
}
