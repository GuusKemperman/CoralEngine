#pragma once

#include "Rendering/DebugRenderer.h"

namespace Engine
{
// Optimizing away debug lines on non editor builds
#ifdef EDITOR
    void DrawDebugLine(
        const World& world,
        DebugCategory::Enum category,
        const glm::vec3& from,
        const glm::vec3& to,
        const glm::vec4& color);

    void DrawDebugLine(
        const World& world,
        DebugCategory::Enum category,
        glm::vec2 from,
        glm::vec2 to,
        const glm::vec4& color,
        Plane::Enum plane = Plane::XZ);

    void DrawDebugCircle(
        const World& world,
        DebugCategory::Enum category,
        const glm::vec3& center,
        float radius,
        const glm::vec4& color,
        Plane::Enum plane = Plane::XZ);

    void DrawDebugSphere(
        const World& world,
        DebugCategory::Enum category,
        const glm::vec3& center,
        float radius,
        const glm::vec4& color);

    void DrawDebugSquare(
        const World& world,
        DebugCategory::Enum category,
        const glm::vec3& center,
        float size,
        const glm::vec4& color,
        Plane::Enum plane = Plane::XZ);

    void DrawDebugBox(
        const World& world,
        DebugCategory::Enum category,
        const glm::vec3& center,
        const glm::vec3& halfExtends,
        const glm::vec4& color);

    void DrawDebugCylinder(
        const World& world,
        DebugCategory::Enum category,
        const glm::vec3& from,
        const glm::vec3& to,
        float radius,
        uint32 segments,
        const glm::vec4& color);

    void DrawDebugPolygon(
        const World& world,
        DebugCategory::Enum category,
        const std::vector<glm::vec3>& points,
        const glm::vec4& color);

    void DrawDebugPolygon(
        const World& world,
        DebugCategory::Enum category,
        const std::vector<glm::vec2>& points,
        const glm::vec4& color,
        Plane::Enum plane = Plane::XZ);
#else
    void DrawDebugLine(
        const World& world,
        DebugCategory::Enum category,
        const glm::vec3& from,
        const glm::vec3& to,
        const glm::vec4& color) {};

    void DrawDebugLine(
        const World& world,
        DebugCategory::Enum category,
        glm::vec2 from,
        glm::vec2 to,
        const glm::vec4& color,
        Plane::Enum plane = Plane::XZ) {};

    void DrawDebugCircle(
        const World& world,
        DebugCategory::Enum category,
        const glm::vec3& center,
        float radius,
        const glm::vec4& color,
        Plane::Enum plane = Plane::XZ) {};

    void DrawDebugSphere(
        const World& world,
        DebugCategory::Enum category,
        const glm::vec3& center,
        float radius,
        const glm::vec4& color) {};

    void DrawDebugSquare(
        const World& world,
        DebugCategory::Enum category,
        const glm::vec3& center,
        float size,
        const glm::vec4& color,
        Plane::Enum plane = Plane::XZ) {};

    void DrawDebugBox(
        const World& world,
        DebugCategory::Enum category,
        const glm::vec3& center,
        const glm::vec3& halfExtends,
        const glm::vec4& color) {};

    void DrawDebugCylinder(
        const World& world,
        DebugCategory::Enum category,
        const glm::vec3& from,
        const glm::vec3& to,
        float radius,
        uint32 segments,
        const glm::vec4& color) {};

    void DrawDebugPolygon(
        const World& world,
        DebugCategory::Enum category,
        const std::vector<glm::vec3>& points,
        const glm::vec4& color) {};

    void DrawDebugPolygon(
        const World& world,
        DebugCategory::Enum category,
        const std::vector<glm::vec2>& points,
        const glm::vec4& color,
        Plane::Enum plane = Plane::XZ) {};
#endif // EDITOR
}