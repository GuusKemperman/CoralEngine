#pragma once
#include "glm/glm.hpp"

namespace Engine {
    class World;
    class DebugRenderer
    {
    public:
        DebugRenderer();
        ~DebugRenderer();
        void AddLine(
            const glm::vec3& from,
            const glm::vec3& to,
            const glm::vec4& color);   
        void AddSphere(
            const glm::vec3& center,
            float radius,
            const glm::vec4& color); 
        void AddCube(
            const glm::vec3& center,
            float size);
        void AddPlane(
            const glm::vec3& p1,
            const glm::vec3& p2);
        void Render(const World& world);

    private:
        class Impl;
        std::unique_ptr<Impl> mImpl;
    };
}

