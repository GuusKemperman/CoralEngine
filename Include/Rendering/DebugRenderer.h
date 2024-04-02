#pragma once

#include "ISubRenderer.h"
#include "glm/glm.hpp"

namespace CE 
{
    class World;

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
            TerrainHeight = 1 << 10,
            All = 0xFFFFFFFF
        };
    };

    struct Plane
    {
        enum Enum
        {
            XY,
            XZ,
            YZ
        };
    };

    class DebugRenderer final :
        public ISubRenderer
    {
    public:
        DebugRenderer();
        ~DebugRenderer() override;
        void Render(const World& world) override;

        void AddLine(
            const World& world,
            DebugCategory::Enum category, 
            const glm::vec3& from, 
            const glm::vec3& to, 
            const glm::vec4& color) const;

        void AddLine(
            const World& world,
            DebugCategory::Enum category, 
            glm::vec2 from, 
            glm::vec2 to, 
            const glm::vec4& color,
            Plane::Enum plane = Plane::XZ) const;

        void AddCircle(
            const World& world,
            DebugCategory::Enum category, 
            const glm::vec3& center, 
            float radius, 
            const glm::vec4& color,
            Plane::Enum plane = Plane::XZ) const;

        void AddSphere(
            const World& world,
            DebugCategory::Enum category, 
            const glm::vec3& center, 
            float radius, 
            const glm::vec4& color) const;

        void AddRectangle(
            const World& world,
            DebugCategory::Enum category, 
            const glm::vec3& center, 
            glm::vec2 halfExtends, 
            const glm::vec4& color,
            Plane::Enum plane = Plane::XZ) const;

        void AddBox(
            const World& world,
            DebugCategory::Enum category, 
            const glm::vec3& center, 
            const glm::vec3& halfExtends, 
            const glm::vec4& color) const;

        void AddCylinder(
            const World& world,
            DebugCategory::Enum category, 
            const glm::vec3& from, 
            const glm::vec3& to, 
            float radius, 
            uint32 segments, 
            const glm::vec4& color) const;

        void AddPolygon(
            const World& world,
            DebugCategory::Enum category, 
            const std::vector<glm::vec3>& points, 
            const glm::vec4& color) const;

        void AddPolygon(
            const World& world,
            DebugCategory::Enum category, 
            const std::vector<glm::vec2>& points, 
            const glm::vec4& color, 
            Plane::Enum plane = Plane::XZ) const;

        static void SetDebugCategoryFlags(DebugCategory::Enum flags) { sDebugCategoryFlags = flags; }
        static DebugCategory::Enum GetDebugCategoryFlags() { return sDebugCategoryFlags; }

        static bool IsCategoryVisible(DebugCategory::Enum flags) { return sDebugCategoryFlags & flags; }

    private:
        class Impl;
        std::unique_ptr<Impl> mImpl;
        static inline DebugCategory::Enum sDebugCategoryFlags{};
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

