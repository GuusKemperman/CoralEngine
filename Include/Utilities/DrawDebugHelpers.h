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

    void DrawDebugRectangle(
        const World& world,
        DebugCategory::Enum category,
        const glm::vec3& center,
        glm::vec2 halfExtends,
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
    inline void DrawDebugLine(
        const World&,
        DebugCategory::Enum,
        const glm::vec3&,
        const glm::vec3&,
        const glm::vec4&) {};

    inline void DrawDebugLine(
        const World&,
        DebugCategory::Enum,
        glm::vec2,
        glm::vec2,
        const glm::vec4&,
        Plane::Enum) {};

    inline void DrawDebugCircle(
        const World&,
        DebugCategory::Enum,
        const glm::vec3&,
        float,
        const glm::vec4&,
        Plane::Enum = Plane::XZ) {};

    inline void DrawDebugSphere(
        const World&,
        DebugCategory::Enum,
        const glm::vec3&,
        float,
        const glm::vec4&) {};

    inline void DrawDebugRectangle(
        const World&,
        DebugCategory::Enum,
        const glm::vec3&,
        glm::vec2,
        const glm::vec4&,
        Plane::Enum = Plane::XZ) {};

    inline void DrawDebugBox(
        const World&,
        DebugCategory::Enum,
        const glm::vec3&,
        const glm::vec3&,
        const glm::vec4&) {};

    inline void DrawDebugCylinder(
        const World&,
        DebugCategory::Enum,
        const glm::vec3&,
        const glm::vec3&,
        float,
        uint32,
        const glm::vec4&) {};

    inline void DrawDebugPolygon(
        const World&,
        DebugCategory::Enum,
        const std::vector<glm::vec3>&,
        const glm::vec4&) {};

    inline void DrawDebugPolygon(
        const World&,
        DebugCategory::Enum,
        const std::vector<glm::vec2>&,
        const glm::vec4&,
        Plane::Enum = Plane::XZ) {};
#endif // EDITOR
}