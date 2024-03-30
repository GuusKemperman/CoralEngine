#pragma once
#include "Rendering/ISubRenderer.h"

class DXPipeline;

namespace Engine
{
    class World;
    class GPUWorld;

    class MeshRenderer final : 
        public ISubRenderer
    {
    public:
        MeshRenderer();
        ~MeshRenderer();
        void Render(const World& world) override;

    private:
        void HandleColorComponent(const World& world, const entt::entity& entity, int meshCounter, int frameIndex);
        void RenderShadowMapsStaticMesh(const World& world, const GPUWorld& gpuWorld);
        //TODO: RenderShadowMapsSkinnedMesh

        std::unique_ptr<DXPipeline> mPBRPipeline;
        std::unique_ptr<DXPipeline> mPBRSkinnedPipeline;
        std::unique_ptr<DXPipeline> mZPipeline;
        std::unique_ptr<DXPipeline> mZSkinnedPipeline;
    };
}