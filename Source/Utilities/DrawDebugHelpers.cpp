#include "Precomp.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Rendering/Renderer.h"

void Engine::DrawDebugLine(const World& world, DebugCategory::Enum category, const glm::vec3& from, const glm::vec3& to, const glm::vec4& color)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddLine(world, category, from, to, color);
}

void Engine::DrawDebugLine(const World& world, DebugCategory::Enum category, glm::vec2 from, glm::vec2 to, const glm::vec4& color, Plane::Enum plane)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddLine(world, category, from, to, color, plane);
}

void Engine::DrawDebugCircle(const World& world, DebugCategory::Enum category, const glm::vec3& center, float radius, const glm::vec4& color, Plane::Enum plane)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddCircle(world, category, center, radius, color, plane);
}

void Engine::DrawDebugSphere(const World& world, DebugCategory::Enum category, const glm::vec3& center, float radius, const glm::vec4& color)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddSphere(world, category, center, radius, color);
}

void Engine::DrawDebugSquare(const World& world, DebugCategory::Enum category, const glm::vec3& center, float size, const glm::vec4& color, Plane::Enum plane)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddSquare(world, category, center, size, color, plane);
}

void Engine::DrawDebugBox(const World& world, DebugCategory::Enum category, const glm::vec3& center, const glm::vec3& halfExtends, const glm::vec4& color)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddBox(world, category, center, halfExtends, color);
}

void Engine::DrawDebugCylinder(const World& world, DebugCategory::Enum category, const glm::vec3& from, const glm::vec3& to, float radius, uint32 segments, const glm::vec4& color)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddCylinder(world, category, from, to, radius, segments, color);
}

void Engine::DrawDebugPolygon(const World& world, DebugCategory::Enum category, const std::vector<glm::vec3>& points, const glm::vec4& color)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddPolygon(world, category, points, color);
}

void Engine::DrawDebugPolygon(const World& world, DebugCategory::Enum category, const std::vector<glm::vec2>& points, const glm::vec4& color, Plane::Enum plane)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddPolygon(world, category, points, color, plane);
}
