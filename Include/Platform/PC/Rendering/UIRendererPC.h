#pragma once

#include "Systems/System.h"
#include "DX12Classes/DXResource.h"
#include "DX12Classes/DXPipeline.h"
#include "DX12Classes/DXConstBuffer.h"

#include "Rendering/ISubRenderer.h"

namespace CE
{
    class Texture;

    class UIRenderer final :
        public ISubRenderer
    {
    public:
        UIRenderer();
        ~UIRenderer();
        void Render(const World& world) override;

    private:
        struct QuadVertex
        {
            glm::vec3 mPosition;
            glm::vec4 mColor;
            glm::vec2 mTexCoord;
            int32 mTextureIndex;
        };

        struct ModelMat
        {
            glm::mat4 mModel;
            glm::mat4 mTransposed;
        };

        struct DrawRequest
        {
            entt::entity mEntity{};
            float mDepth{};
            uint16 mQuadIndex;
            bool mIsText;
        };


        void RenderUI(const World& world, const DrawRequest& drawRequest);
        void RenderText(const World& world, const DrawRequest& drawRequest);
        void ProcessDrawRequests(const World& world);

    private:
        ComPtr<ID3D12PipelineState> mPipeline;
        ComPtr<ID3D12PipelineState> mTextPipeline;
        std::vector<DrawRequest> mDrawRequests;
    };
}