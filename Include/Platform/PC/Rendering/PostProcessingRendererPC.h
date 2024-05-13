#pragma once
#include "Rendering/ISubRenderer.h"
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"

namespace CE
{
    class World;

    class PostProcessingRenderer final :
        public ISubRenderer
    {
    public:
        PostProcessingRenderer();
        ~PostProcessingRenderer();
        void Render(const World& world) override;

    private:
        void RenderOutline(const World& world);
        ComPtr<ID3D12PipelineState>  mOutlinePipeline;

    };
};