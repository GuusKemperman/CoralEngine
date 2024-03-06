#pragma once

#include "glm/glm.hpp"
#include "Core/EngineSubsystem.h"
#include "World/World.h"

namespace Engine 
{
    class DebugRenderer final :
        public EngineSubsystem<DebugRenderer>
    {
        friend EngineSubsystem;
        DebugRenderer();
        ~DebugRenderer();

    public:
        void Render(const World& world);
        void AddLine(
            const glm::vec3& from,
            const glm::vec3& to,
            const glm::vec4& color);

        /// <summary>
        /// Renders a circle with debug lines. Assumes y = center.y.
        /// </summary>
        /// <param name="center">Center of the circle.</param>
        /// <param name="radius">Rdius of the circle.</param>
        /// <param name="color">Color of the lines.</param>
        void AddCircle(
            const glm::vec3& center,
            float radius,
            const glm::vec4& color);
        void AddSphere(
            const glm::vec3& center,
            float radius,
            const glm::vec4& color); 
        void AddCube(
            const glm::vec3& center,
            float size,
            const glm::vec4& color);

        /// <summary>
        /// Plane drawn from lines. Will assume plane is renderer flatly on the y axis.
        /// </summary>
        /// <param name="center">Center of the plane.</param>
        /// <param name="size">Size of the plane.</param>
        /// <param name="color">Color of the lines.</param>
        void AddPlane(
            const glm::vec3& center,
            float size,
            const glm::vec4& color);

    private:
        class Impl;
        std::unique_ptr<Impl> mImpl;
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

