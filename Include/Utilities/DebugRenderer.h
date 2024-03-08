#pragma once

#include "glm/glm.hpp"
#include "World/World.h"

namespace Engine 
{
    struct DebugCategory
    {
        enum Enum
        {
            General = 1 << 0,
            Gameplay = 1 << 1,
            Physics = 1 << 2,
            Sound = 1 << 3,
            Rendering = 1 << 4,
            AINavigation = 1 << 5,
            AIDecision = 1 << 6,
            Editor = 1 << 7,
            AccelStructs = 1 << 8,
            Particles = 1 << 9,
            All = 0xFFFFFFFF
        };
    };

    struct Plane
    {
        enum Enum
        {
            XY = 0,
            XZ,
            YZ
        };
    };

    class DebugRenderer
    {
        friend class WorldRenderer;
        
    public:
        DebugRenderer();
        ~DebugRenderer();
        void Render(const World& world);

        void AddLine(
            DebugCategory::Enum category, 
            const glm::vec3& from, 
            const glm::vec3& to, 
            const glm::vec4& color) const;

        void AddLine(
            DebugCategory::Enum category, 
            const glm::vec2& from, 
            const glm::vec2& to, 
            const glm::vec4& color,
            Plane::Enum plane = Plane::XZ) const;

        void AddCircle(
            DebugCategory::Enum category, 
            const glm::vec3& center, 
            float radius, 
            const glm::vec4& color,
            Plane::Enum plane = Plane::XZ) const;

        void AddSphere(
            DebugCategory::Enum category, 
            const glm::vec3& center, 
            float radius, 
            const glm::vec4& color) const;

        void AddSquare(
            DebugCategory::Enum category, 
            const glm::vec3& center, 
            float size, 
            const glm::vec4& color,
            Plane::Enum plane = Plane::XZ) const;

        void AddBox(
            DebugCategory::Enum category, 
            const glm::vec3& center, 
            const glm::vec3& halfExtends, 
            const glm::vec4& color) const;

        void AddPolygon(
            DebugCategory::Enum category, 
            const std::vector<glm::vec3>& points, 
            const glm::vec4& color) const;

        void AddPolygon(
            DebugCategory::Enum category, 
            const std::vector<glm::vec2>& points, 
            const glm::vec4& color, 
            Plane::Enum plane = Plane::XZ) const;

        static void SetDebugCategoryFlags(DebugCategory::Enum flags) { sDebugCategoryFlags = flags; }
        static DebugCategory::Enum GetDebugCategoryFlags() { return sDebugCategoryFlags; }

    private:
        class Impl;
        std::unique_ptr<Impl> mImpl;
        static inline DebugCategory::Enum sDebugCategoryFlags{ DebugCategory::All };
    };

    namespace Colors
    {
        inline glm::vec4 Black = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        inline glm::vec4 White = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        inline glm::vec4 Grey = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        inline glm::vec4 Red = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        inline glm::vec4 Green = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        inline glm::vec4 Blue = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        inline glm::vec4 Orange = glm::vec4(1.0f, 0.66f, 0.0f, 1.0f);
        inline glm::vec4 Cyan = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        inline glm::vec4 Magenta = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
        inline glm::vec4 Yellow = glm::vec4(1.0f, 1.0, 0.0f, 1.0f);
        inline glm::vec4 Purple = glm::vec4(0.55f, 0.0, 0.65f, 1.0f);
    }
}

