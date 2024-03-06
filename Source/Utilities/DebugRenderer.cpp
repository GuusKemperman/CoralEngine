#include "Precomp.h"
#include "Utilities/DebugRenderer.h"

#include <glm/gtc/constants.hpp>

void Engine::DebugRenderer::AddCircle(const glm::vec3& center, float radius, const glm::vec4& color)
{
    constexpr float dt = glm::two_pi<float>() / 32.0f;
    float t = 0.0f;

    glm::vec3 v0(center.x + radius * cos(t), center.y + radius * sin(t), center.z);
    for (; t < glm::two_pi<float>(); t += dt)
    {
        glm::vec3 v1(center.x + radius * cos(t + dt), center.y, center.z + radius * sin(t + dt));
        AddLine(v0, v1, color);
        v0 = v1;
    }
}

void Engine::DebugRenderer::AddSphere(const glm::vec3& center, float radius, const glm::vec4& color)
{
    constexpr float dt = glm::two_pi<float>() / 64.0f;

    // Draw circle on x axis
    float t = 0.0f;
    glm::vec3 v0(center.x + radius * cos(t), center.y + radius * sin(t), center.z);

    for (; t < glm::two_pi<float>() - dt; t += dt)
    {
        glm::vec3 v1(center.x + radius * cos(t + dt), center.y + radius * sin(t + dt), center.z);
        AddLine(v0, v1, color);
        v0 = v1;
    }

    // Draw circle on z axis
    t = 0.0f;
    v0 = glm::vec3(center.x, center.y + radius * sin(t), center.z + radius * cos(t));

    for (; t < glm::two_pi<float>() - dt; t += dt)
    {
        glm::vec3 v1(center.x, center.y + radius * sin(t + dt), center.z + radius * cos(t + dt));
        AddLine(v0, v1, color);
        v0 = v1;
    }
}

void Engine::DebugRenderer::AddCube(const glm::vec3& center, float size, const glm::vec4& color)
{
    glm::vec3 min = center - size;
    glm::vec3 max = center + size;

    glm::vec3 lengthX = glm::vec3(max.x - min.x, 0.0f, 0.0f);
    glm::vec3 lengthY = glm::vec3(0.0f, max.y - min.y, 0.0f);
    glm::vec3 lengthZ = glm::vec3(0.0f, 0.0f, max.z - min.z);

    // Bottom
    AddLine(min, max - lengthY - lengthX, color);
    AddLine(min, max - lengthY - lengthZ, color);
    AddLine(min + lengthX, max - lengthY, color);
    AddLine(min + lengthZ, max - lengthY, color);

    // Top
    AddLine(min + lengthY, max - lengthX, color);
    AddLine(min + lengthY, max - lengthZ, color);
    AddLine(min + lengthY + lengthX, max, color);
    AddLine(min + lengthY + lengthZ, max, color);

    // Sides
    AddLine(min, max - lengthX - lengthZ, color);
    AddLine(min + lengthX, max - lengthZ, color);
    AddLine(min + lengthZ, max - lengthX, color);
    AddLine(min + lengthX + lengthZ, max, color);
}

void Engine::DebugRenderer::AddPlane(const glm::vec3& center, float size, const glm::vec4& color)
{
    glm::vec3 p1 = glm::vec3(center.x - size, center.y, center.z - size);
    glm::vec3 p2 = glm::vec3(center.x + size, center.y, center.z + size);
    glm::vec3 p3 = glm::vec3(p2.x, p1.y, p1.z);
    glm::vec3 p4 = glm::vec3(p1.x, p1.y, p2.z);

    // Square
    AddLine(p1, p3, color);
    AddLine(p1, p4, color);
    AddLine(p2, p3, color);
    AddLine(p2, p4, color);

    // Cross for better visibility
    AddLine(p1, p2, color);
    AddLine(p3, p4, color);
}