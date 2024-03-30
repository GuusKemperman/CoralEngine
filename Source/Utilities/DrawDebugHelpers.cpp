#include "Precomp.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Rendering/Renderer.h"

#ifdef EDITOR
void CE::DrawDebugLine(const World& world, DebugCategory::Enum category, const glm::vec3& from, const glm::vec3& to, const glm::vec4& color)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddLine(world, category, from, to, color);
}

void CE::DrawDebugLine(const World& world, DebugCategory::Enum category, glm::vec2 from, glm::vec2 to, const glm::vec4& color, Plane::Enum plane)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddLine(world, category, from, to, color, plane);
}

void CE::DrawDebugCircle(const World& world, DebugCategory::Enum category, const glm::vec3& center, float radius, const glm::vec4& color, Plane::Enum plane)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddCircle(world, category, center, radius, color, plane);
}

void CE::DrawDebugSphere(const World& world, DebugCategory::Enum category, const glm::vec3& center, float radius, const glm::vec4& color)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddSphere(world, category, center, radius, color);
}

void CE::DrawDebugRectangle(const World& world, DebugCategory::Enum category,
	const glm::vec3& center,
	glm::vec2 halfExtends,
	const glm::vec4& color,
	Plane::Enum plane)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddRectangle(world, category, center, halfExtends, color, plane);
}

void CE::DrawDebugBox(const World& world, DebugCategory::Enum category, const glm::vec3& center, const glm::vec3& halfExtends, const glm::vec4& color)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddBox(world, category, center, halfExtends, color);
}

void CE::DrawDebugCylinder(const World& world, DebugCategory::Enum category, const glm::vec3& from, const glm::vec3& to, float radius, uint32 segments, const glm::vec4& color)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddCylinder(world, category, from, to, radius, segments, color);
}

void CE::DrawDebugPolygon(const World& world, DebugCategory::Enum category, const std::vector<glm::vec3>& points, const glm::vec4& color)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddPolygon(world, category, points, color);
}

void CE::DrawDebugPolygon(const World& world, DebugCategory::Enum category, const std::vector<glm::vec2>& points, const glm::vec4& color, Plane::Enum plane)
{
	const DebugRenderer& renderer = Renderer::Get().GetDebugRenderer();
	renderer.AddPolygon(world, category, points, color, plane);
}
#endif // EDITOR
