#pragma once
#include "Rendering/IGPUWorld.h"
#include "DX12Classes/DXDefines.h"
#include "Platform/PC/Rendering/DX12Classes/DXHeapHandle.h"

class DXResource;
class DXConstBuffer;
class DXDescHeap;

namespace Engine
{
    class Material;

    namespace InfoStruct 
    {
        struct DXMatrixInfo 
        {
            glm::mat4x4 vm;
            glm::mat4x4 pm;
            glm::mat4x4 ivm;
            glm::mat4x4 ipm;
        };

        struct DXDirLightInfo 
        {
            glm::vec4 mDir = { 0.f, 0.0f, 0.0f, 0.f };
            glm::vec4 mColorAndIntensity = { 0.f, 0.0f, 0.0f, 0.f };
        };

        struct DXPointLightInfo 
        {
            glm::vec4 mPosition = { 0.f, 0.0f, 0.0f, 0.f };
            glm::vec4 mColorAndIntensity = { 0.f, 0.0f, 0.0f, 0.f };
            float mRadius = 0.f;
            float padding[3];
        };

        struct DXLightInfo 
        {
            DXPointLightInfo mPointLights[MAX_LIGHTS];
            DXDirLightInfo mDirLights[MAX_LIGHTS];
        };

        struct DXMaterialInfo
        {
            glm::vec4 colorFactor;
            glm::vec4 emissiveFactor;
            float metallicFactor;
            float roughnessFactor;
            float normalScale;
            unsigned int useColorTex;
            unsigned int useEmissiveTex;
            unsigned int useMetallicRoughnessTex;
            unsigned int useNormalTex;
            unsigned int useOcclusionTex;
        };

        struct DXColorMultiplierInfo
        {
            glm::vec4 colorMult;
            glm::vec4 colorAdd;
        };

        enum DXStructuredBuffers 
        {
            MODEL_MAT_SB,
            MATERIAL_SB
        };

        struct ColorInfo
        {
            glm::vec4 mColor;
            uint32 mUseTexture;
            uint32 mPadding[3];
        };
    }

    class DebugRenderingData
    {
    public:
        DebugRenderingData();
        ~DebugRenderingData();

        D3D12_VERTEX_BUFFER_VIEW mVertexPositionBufferView;
        D3D12_VERTEX_BUFFER_VIEW mVertexColorBufferView;
        std::unique_ptr<DXResource> mVertexPositionBuffer;
        std::unique_ptr<DXResource> mVertexColorBuffer;
        std::vector<glm::vec3> mPositions{};
        std::vector<glm::vec4> mColors{};
        uint32 mLineCount = 0;
    };

    class UIRenderingData 
    {
    public:
        UIRenderingData();

        std::unique_ptr<DXResource> mQuadVResource;
        std::unique_ptr<DXResource> mQuadUVResource;
        std::unique_ptr<DXResource> mIndicesResource;

        std::unique_ptr<DXConstBuffer> mModelMatBuffer;
        std::unique_ptr<DXConstBuffer> mColorBuffer;

        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
        D3D12_VERTEX_BUFFER_VIEW mTexCoordBufferView;
        D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
    };

    class UIRenderingData {
    public:
        UIRenderingData();

        std::unique_ptr<DXResource> mQuadVResource;
        std::unique_ptr<DXResource> mQuadUVResource;
        std::unique_ptr<DXResource> mIndicesResource;

        std::unique_ptr<DXConstBuffer> mModelMatBuffer;
        std::unique_ptr<DXConstBuffer> mColorBuffer;

        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
        D3D12_VERTEX_BUFFER_VIEW mTexCoordBufferView;
        D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
    };

	class GPUWorld final : 
		public IGPUWorld
	{
	public:
        GPUWorld(const World& world);
		~GPUWorld();
		void Update() override;

        /// <summary>
        /// Updating the material buffer has to happen after the mesh rendering commands have completed.
        /// </summary>
        void UpdateMaterials();

        const DXConstBuffer& GetCameraBuffer() const { return *mConstBuffers[CAM_MATRIX_CB]; };
        DXConstBuffer& GetModelIndexBuffer() const { return *mConstBuffers[MODEL_INDEX_CB]; };
        const DXConstBuffer& GetLightBuffer() const { return *mConstBuffers[LIGHT_CB]; };
        DXConstBuffer& GetModelMatrixBuffer() const { return *mConstBuffers[MODEL_MATRIX_CB]; };
        DXConstBuffer& GetBoneMatrixBuffer() const { return *mConstBuffers[FINAL_BONE_MATRIX_CB]; };
        DXConstBuffer& GetMeshColorBuffer() const { return *mConstBuffers[COLOR_CB]; };

        const InfoStruct::DXMaterialInfo& GetMaterial(int meshIndex) const { return mMaterials[meshIndex]; };
        const DXHeapHandle& GetMaterialHeapSlot() const { return mMaterialHeapSlot; };

        DebugRenderingData& GetDebugRenderingData() { return mDebugRenderingData; };
        UIRenderingData& GetUIRenderingData() { return mUIRenderingData; };

	private:
        void SendMaterialTexturesToGPU(const Material& mat);

		std::unique_ptr<DXConstBuffer> mConstBuffers[NUM_CBS];
		std::unique_ptr<DXResource> mStructuredBuffers[3];
		InfoStruct::DXLightInfo mLights;
		DXHeapHandle mMaterialHeapSlot;
		std::vector<InfoStruct::DXMaterialInfo> mMaterials;

        DebugRenderingData mDebugRenderingData;
        UIRenderingData mUIRenderingData;
	};
}

