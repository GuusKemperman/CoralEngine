#pragma once

#include "Systems/System.h"
#include "DX12Classes/DXResource.h"
#include "DX12Classes/DXPipeline.h"
#include "DX12Classes/DXConstBuffer.h"

#include "Rendering/ISubRenderer.h"

namespace Engine
{
    class Texture;

    class UIRenderer final :
        public ISubRenderer
    {
    public:
        UIRenderer();
        ~UIRenderer();
        void Render(const World& world) override;

        SystemStaticTraits GetStaticTraits() const override
        {
            SystemStaticTraits traits{};
            traits.mPriority = static_cast<int>(TickPriorities::PostRender) + (TickPriorityStepSize >> 1);
            return traits;
        }

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

        struct ColorInfo
        {
            glm::vec4 mColor;
            uint32 mUseTexture;
            uint32 mPadding[3];
        };

    private:
        std::unique_ptr<DXPipeline> mPipeline;
        std::unique_ptr<DXResource> mQuadVResource;
        std::unique_ptr<DXResource> mQuadUVResource;
        std::unique_ptr<DXResource> mIndicesResource;
        
        std::vector<glm::mat4x4> mModelVector;
        std::unique_ptr<DXConstBuffer> mModelMatBuffer;
        std::unique_ptr<DXConstBuffer> mColorBuffer;

        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
        D3D12_VERTEX_BUFFER_VIEW mTexCoordBufferView;
        D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

        friend ReflectAccess;
        static MetaType Reflect();
    };
}