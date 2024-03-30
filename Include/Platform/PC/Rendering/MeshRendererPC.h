#pragma once
#include "Rendering/ISubRenderer.h"

class DXPipeline;

namespace CE
{
    class World;

    class MeshRenderer final : 
        public ISubRenderer
    {
    public:
        MeshRenderer();
        ~MeshRenderer();
        void Render(const World& world) override;

    private:
        void HandleColorComponent(const World& world, const entt::entity& entity, int meshCounter, int frameIndex);

        std::unique_ptr<DXPipeline> mPBRPipeline;
        std::unique_ptr<DXPipeline> mPBRSkinnedPipeline;
        std::unique_ptr<DXPipeline> mZPipeline;
        std::unique_ptr<DXPipeline> mZSkinnedPipeline;
    };
}