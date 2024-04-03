#pragma once
#include "Rendering/ISubRenderer.h"
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"

class DXPipelineBuilder;

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

        ComPtr<ID3D12PipelineState> mPBRPipeline;
        ComPtr<ID3D12PipelineState> mPBRSkinnedPipeline;
        ComPtr<ID3D12PipelineState> mZPipeline;
        ComPtr<ID3D12PipelineState> mZSkinnedPipeline;
    };
}