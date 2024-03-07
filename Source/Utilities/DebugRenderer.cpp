#include "Precomp.h"
#include "Utilities/DebugRenderer.h"

void Engine::DebugRenderer::AddLine(DebugCategory::Enum category, const glm::vec2& from, const glm::vec2& to, const glm::vec4& color, Plane::Enum plane) const
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

void Engine::DebugRenderer::AddCircle(DebugCategory::Enum category, const glm::vec3& center, float radius, const glm::vec4& color, Plane::Enum plane) const
{
    if (!(sDebugCategoryFlags & category)) return;

    constexpr float dt = glm::two_pi<float>() / 64.0f;
    float t = 0.0f;
    glm::vec3 v0{};

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

void Engine::DebugRenderer::AddSphere(DebugCategory::Enum category, const glm::vec3& center, float radius, const glm::vec4& color) const
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

void Engine::DebugRenderer::AddSquare(DebugCategory::Enum category, const glm::vec3& center, float size, const glm::vec4& color, Plane::Enum plane) const
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

void Engine::DebugRenderer::AddBox(DebugCategory::Enum category, const glm::vec3& center, const glm::vec3& halfExtents, const glm::vec4& color) const
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

void Engine::DebugRenderer::AddPolygon(DebugCategory::Enum category, const std::vector<glm::vec3>& points, const glm::vec4& color) const
{
    if (!(sDebugCategoryFlags & category)) return;

    const size_t pointCount = points.size();
    for (size_t i = 0; i < pointCount; i++)
    {
        AddLine(category, points[i], points[(i + 1) % pointCount], color);
    }
}

void Engine::DebugRenderer::AddPolygon(DebugCategory::Enum category, const std::vector<glm::vec2>& points, const glm::vec4& color, Plane::Enum plane) const
{
    if (!(sDebugCategoryFlags & category)) return;

    const size_t pointCount = points.size();

    switch (plane)
    {
    case Plane::XY:
        for (size_t i = 0; i < pointCount; i++)
        {
            const size_t nextPointIndex = (i + 1) % pointCount;
            AddLine(category, { points[i].x, points[i].y, 0.f }, { points[nextPointIndex].x, points[nextPointIndex].y, 0.f }, color);
        }
        break;
    case Plane::XZ:
        for (size_t i = 0; i < pointCount; i++)
        {
            const size_t nextPointIndex = (i + 1) % pointCount;
            AddLine(category, { points[i].x, 0.f, points[i].y }, { points[nextPointIndex].x, 0.f , points[nextPointIndex].y}, color);
        }
        break;
    case Plane::YZ:
        for (size_t i = 0; i < pointCount; i++)
        {
            const size_t nextPointIndex = (i + 1) % pointCount;
            AddLine(category, { 0.f, points[i].x, points[i].y }, { 0.f, points[nextPointIndex].x, points[nextPointIndex].y }, color);
        }
        break;
    }
}