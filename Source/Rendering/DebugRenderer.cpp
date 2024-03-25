#include "Precomp.h"
#include "Rendering/DebugRenderer.h"
#include "glm/gtx/rotate_vector.hpp"

void Engine::DebugRenderer::AddLine(const World& world, DebugCategory::Enum category, const glm::vec2& from, const glm::vec2& to, const glm::vec4& color, Plane::Enum plane) const
{
    if (!IsCategoryVisible(category)) return;

    switch (plane)
    {
    case Plane::XY:
        AddLine(world, category, glm::vec3(from.x, from.y, 0.0f), glm::vec3(to.x, to.y, 0.0f), color);
        break;
    case Plane::XZ:
        AddLine(world, category, glm::vec3(from.x, 0.0f, from.y), glm::vec3(to.x, 0.0f, to.y), color);
        break;
    case Plane::YZ:
        AddLine(world, category, glm::vec3(0.0f, from.x, from.y), glm::vec3(0.0f, to.x, to.y), color);
        break;
    }
}

void Engine::DebugRenderer::AddCircle(const World& world, DebugCategory::Enum category, const glm::vec3& center, float radius, const glm::vec4& color, Plane::Enum plane) const
{
    if (!IsCategoryVisible(category)) return;

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
            AddLine(world, category, v0, v1, color);
            v0 = v1;
        }
        break;

    case Plane::XZ:

        v0 = { center.x + radius * cos(t), center.y, center.z + radius * sin(t) };
        for (; t < glm::two_pi<float>() - dt; t += dt)
        {
	        glm::vec3 v1(center.x + radius * cos(t + dt), center.y, center.z + radius * sin(t + dt));
            AddLine(world, category, v0, v1, color);
            v0 = v1;
        }
        break;

    case Plane::YZ:

        v0 = { center.x, center.y + radius * cos(t), center.z + radius * sin(t) };

        for (; t < glm::two_pi<float>() - dt; t += dt)

        {
	        glm::vec3 v1(center.x, center.y + radius * cos(t + dt), center.z + radius * sin(t + dt));
            AddLine(world, category, v0, v1, color);
            v0 = v1;
        }

        break;
    }
}

void Engine::DebugRenderer::AddSphere(const World& world, DebugCategory::Enum category, const glm::vec3& center, float radius, const glm::vec4& color) const
{
    if (!IsCategoryVisible(category)) return;

    constexpr float dt = glm::two_pi<float>() / 64.0f;
    float t = 0.0f;
    glm::vec3 v0;

    // xy
    v0 = { center.x + radius * cos(t), center.y + radius * sin(t), center.z };
    for (; t < glm::two_pi<float>() - dt; t += dt)
    {
	    glm::vec3 v1(center.x + radius * cos(t + dt), center.y + radius * sin(t + dt), center.z);
        AddLine(world, category, v0, v1, color);
        v0 = v1;
    }
    // xz
    t = 0.0f;
    v0 = { center.x + radius * cos(t), center.y, center.z + radius * sin(t) };
    for (; t < glm::two_pi<float>() - dt; t += dt)
    {
	    glm::vec3 v1(center.x + radius * cos(t + dt), center.y, center.z + radius * sin(t + dt));
        AddLine(world, category, v0, v1, color);
        v0 = v1;
    }
    // yz
    t = 0.0f;
    v0 = { center.x, center.y + radius * cos(t), center.z + radius * sin(t) };
    for (; t < glm::two_pi<float>() - dt; t += dt)
    {
	    glm::vec3 v1(center.x, center.y + radius * cos(t + dt), center.z + radius * sin(t + dt));
        AddLine(world, category, v0, v1, color);
        v0 = v1;
    }
}

void Engine::DebugRenderer::AddSquare(const World& world, DebugCategory::Enum category, const glm::vec3& center, float size, const glm::vec4& color, Plane::Enum plane) const
{
    if (!IsCategoryVisible(category)) return;

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

    AddLine(world, category, A, B, color);
    AddLine(world, category, B, C, color);
    AddLine(world, category, C, D, color);
    AddLine(world, category, D, A, color);
}

void Engine::DebugRenderer::AddBox(const World& world, DebugCategory::Enum category, const glm::vec3& center, const glm::vec3& halfExtents, const glm::vec4& color) const
{
    if (!IsCategoryVisible(category)) return;

    // Generated using chatgpt

    // Calculate the minimum and maximum corner points of the box
    const glm::vec3 minCorner = center - halfExtents;
    const glm::vec3 maxCorner = center + halfExtents;

    // Generate the lines for the box
    // Front face
    AddLine(world, category, glm::vec3(minCorner.x, minCorner.y, minCorner.z), glm::vec3(maxCorner.x, minCorner.y, minCorner.z), color);
    AddLine(world, category, glm::vec3(maxCorner.x, minCorner.y, minCorner.z), glm::vec3(maxCorner.x, maxCorner.y, minCorner.z), color);
    AddLine(world, category, glm::vec3(maxCorner.x, maxCorner.y, minCorner.z), glm::vec3(minCorner.x, maxCorner.y, minCorner.z), color);
    AddLine(world, category, glm::vec3(minCorner.x, maxCorner.y, minCorner.z), glm::vec3(minCorner.x, minCorner.y, minCorner.z), color);

    // Back face
    AddLine(world, category, glm::vec3(minCorner.x, minCorner.y, maxCorner.z), glm::vec3(maxCorner.x, minCorner.y, maxCorner.z), color);
    AddLine(world, category, glm::vec3(maxCorner.x, minCorner.y, maxCorner.z), glm::vec3(maxCorner.x, maxCorner.y, maxCorner.z), color);
    AddLine(world, category, glm::vec3(maxCorner.x, maxCorner.y, maxCorner.z), glm::vec3(minCorner.x, maxCorner.y, maxCorner.z), color);
    AddLine(world, category, glm::vec3(minCorner.x, maxCorner.y, maxCorner.z), glm::vec3(minCorner.x, minCorner.y, maxCorner.z), color);

    // Connecting lines between the front and back faces
    AddLine(world, category, glm::vec3(minCorner.x, minCorner.y, minCorner.z), glm::vec3(minCorner.x, minCorner.y, maxCorner.z), color);
    AddLine(world, category, glm::vec3(maxCorner.x, minCorner.y, minCorner.z), glm::vec3(maxCorner.x, minCorner.y, maxCorner.z), color);
    AddLine(world, category, glm::vec3(maxCorner.x, maxCorner.y, minCorner.z), glm::vec3(maxCorner.x, maxCorner.y, maxCorner.z), color);
    AddLine(world, category, glm::vec3(minCorner.x, maxCorner.y, minCorner.z), glm::vec3(minCorner.x, maxCorner.y, maxCorner.z), color);
}

void Engine::DebugRenderer::AddCylinder(const World& world, DebugCategory::Enum category, const glm::vec3& from, const glm::vec3& to, float radius, uint32 segments, const glm::vec4& color) const
{
    if (!IsCategoryVisible(category)) return;

    segments = segments < 4 ? 4 : segments; // We need at least 4 segments

    // Rotate a point around axis to form cylinder segments
    const float angleIncrease = glm::radians(360.0f / segments);
    float angle = angleIncrease;
    glm::vec3 axis = glm::normalize(to - from);
    
    glm::vec3 perpendicular{};
    glm::vec3 axisAbs = glm::abs(axis);
    
    // Find best basis vectors
    if (axisAbs.z > axisAbs.x 
        && axisAbs.z > axisAbs.y)
    {
        perpendicular = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    else
    {
        perpendicular = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    perpendicular = glm::normalize(perpendicular - axis * glm::vec3(perpendicular.x * axis.x + perpendicular.y * axis.y + perpendicular.z * axis.z));
    glm::vec3 segment = glm::rotate(perpendicular, 0.0f, axis) * radius;
    
    glm::vec3 p1 = segment + from;
    glm::vec3 p3 = segment + to;
    
    for (uint32 i = 0; i < segments; ++i)
    {
        segment = glm::rotate(perpendicular, angle, axis) * radius;
        glm::vec3 p2 = segment + from;
        glm::vec3 p4 = segment + to;
        
        AddLine(world, category, p2, p4, color);
        AddLine(world, category, p1, p2, color);
        AddLine(world, category, p3, p4, color);

        p1 = p2;
        p3 = p4;
        angle += angleIncrease;
    }
}

void Engine::DebugRenderer::AddPolygon(const World& world, DebugCategory::Enum category, const std::vector<glm::vec3>& points, const glm::vec4& color) const
{
    if (!IsCategoryVisible(category)) return;

    const size_t pointCount = points.size();
    for (size_t i = 0; i < pointCount; i++)
    {
        AddLine(world, category, points[i], points[(i + 1) % pointCount], color);
    }
}

void Engine::DebugRenderer::AddPolygon(const World& world, DebugCategory::Enum category, const std::vector<glm::vec2>& points, const glm::vec4& color, Plane::Enum plane) const
{
    if (!IsCategoryVisible(category)) return;

    const size_t pointCount = points.size();

    switch (plane)
    {
    case Plane::XY:
        for (size_t i = 0; i < pointCount; i++)
        {
            const size_t nextPointIndex = (i + 1) % pointCount;
            AddLine(world, category, { points[i].x, points[i].y, 0.f }, { points[nextPointIndex].x, points[nextPointIndex].y, 0.f }, color);
        }
        break;
    case Plane::XZ:
        for (size_t i = 0; i < pointCount; i++)
        {
            const size_t nextPointIndex = (i + 1) % pointCount;
            AddLine(world, category, { points[i].x, 0.f, points[i].y }, { points[nextPointIndex].x, 0.f , points[nextPointIndex].y}, color);
        }
        break;
    case Plane::YZ:
        for (size_t i = 0; i < pointCount; i++)
        {
            const size_t nextPointIndex = (i + 1) % pointCount;
            AddLine(world, category, { 0.f, points[i].x, points[i].y }, { 0.f, points[nextPointIndex].x, points[nextPointIndex].y }, color);
        }
        break;
    }
}