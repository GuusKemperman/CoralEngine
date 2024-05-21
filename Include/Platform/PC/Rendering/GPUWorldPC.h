#pragma once
#include "Rendering/IGPUWorld.h"
#include "DX12Classes/DXDefines.h"
#include "Platform/PC/Rendering/DX12Classes/DXHeapHandle.h"
#include "Platform/PC/Rendering/InfoStruct.h"

class DXResource;
class DXConstBuffer;
class DXDescHeap;

namespace CE
{
    class Material;
    class CameraComponent;
    class FrameBuffer;

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

    class PosProcRenderingData
    {
    public:
        PosProcRenderingData();
        void Update(const World& world);

        std::unique_ptr<DXResource> mQuadVResource;
        std::unique_ptr<DXResource> mQuadUVResource;
        std::unique_ptr<DXResource> mIndicesResource;

        std::unique_ptr<DXConstBuffer> mOutlineBuffer;

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
        void UpdateLights(int numDirLights, int numPointLights);
        void ClearClusterData();

        DXResource& GetStructuredBuffer(InfoStruct::DXStructuredBuffers structuredBuffer) const { return *mStructuredBuffers[structuredBuffer]; }
        const DXConstBuffer& GetConstantBuffer(InfoStruct::DXConstantBuffers constantBuffer) const { return *mConstBuffers[constantBuffer]; }
        const DXConstBuffer& GetCameraBuffer() const { return *mConstBuffers[InfoStruct::CAM_MATRIX_CB]; };
        DXConstBuffer& GetMaterialInfoBuffer() const { return *mConstBuffers[InfoStruct::MATERIAL_INFO_CB]; };
        const DXConstBuffer& GetLightBuffer() const { return *mConstBuffers[InfoStruct::LIGHT_CB]; };
        DXConstBuffer& GetModelMatrixBuffer() const { return *mConstBuffers[InfoStruct::MODEL_MATRIX_CB]; };
        DXConstBuffer& GetBoneMatrixBuffer() const { return *mConstBuffers[InfoStruct::FINAL_BONE_MATRIX_CB]; };
        DXConstBuffer& GetMeshColorBuffer() const { return *mConstBuffers[InfoStruct::COLOR_CB]; };

        const DXHeapHandle& GetDirLightHeapSlot() const { return mDirectionalLightsSRVSlot; };
        const DXHeapHandle& GetPointLigthHeapSlot() const { return mPointLightsSRVSlot; };
        const DXHeapHandle& GetCompactClusterSRVSlot() const { return mCompactClusterSRVSlot; };
        const DXHeapHandle& GetClusterSRVSlot() const { return mClusterSRVSlot; };
        const DXHeapHandle& GetActiveClusterSRVSlot() const { return mActiveClusterSRVSlot; };
        const DXHeapHandle& GetLightIndicesSRVSlot() const { return mLightIndicesSRVSlot; };
        const DXHeapHandle& GetLigthGridSRVSlot() const { return mLightGridSRVSlot; };

        const DXHeapHandle& GetCompactClusterUAVSlot() const { return mCompactClusterUAVSlot; };
        const DXHeapHandle& GetClusterUAVSlot() const { return mClusterUAVSlot; };
        const DXHeapHandle& GetActiveClusterUAVSlot() const { return mActiveClusterUAVSlot; };
        const DXHeapHandle& GetLightIndicesUAVSlot() const { return mLightIndicesUAVSlot; };
        const DXHeapHandle& GetLigthGridUAVSlot() const { return mLightGridUAVSlot; };
        const DXHeapHandle& GetPointLightCounterUAVSlot() const { return mPointLightCounterUAVSlot; };

        const InfoStruct::DXShadowMapInfo* GetShadowMap() const { return mShadowMap.get(); }
        const InfoStruct::DXParticleInfo& GetParticle(int i) const { return mParticles[i]; }
        const int GetNumParticles() const { return mParticleCount; }

        DebugRenderingData& GetDebugRenderingData() { return mDebugRenderingData; };
        UIRenderingData& GetUIRenderingData() { return mUIRenderingData; };
        PosProcRenderingData& GetPostProcData() { return mPostProcData; };
        FrameBuffer& GetSelectionFramebuffer() const { return *mSelectedMeshFrameBuffer; }

        glm::ivec3 GetClusterGrid() const { return mClusterGrid; }
        int GetNumberOfClusters() const { return mNumberOfClusters; }
        uint32 ReadCompactClusterCounter() const;

	private:
        void UpdateClusterData(const CameraComponent& camera);
        void InitializeShadowMaps();
        void UpdateParticles(glm::vec3 cameraPos);

        InfoStruct::DXMaterialInfo GetMaterial(const CE::Material* material);

		std::unique_ptr<DXConstBuffer> mConstBuffers[InfoStruct::NUM_CBS];
		std::unique_ptr<DXResource> mStructuredBuffers[InfoStruct::NUM_SB];

        InfoStruct::DXLightInfo mLightInfo;

        std::vector<InfoStruct::DXDirLightInfo> mDirectionalLights;
        std::vector<InfoStruct::DXPointLightInfo> mPointLights;
        std::vector<InfoStruct::DXParticleInfo> mParticles;
        int mParticleCount = 0;
        int mPointLightCounter = 0;

        std::unique_ptr<InfoStruct::DXShadowMapInfo> mShadowMap;
        std::unique_ptr<FrameBuffer> mSelectedMeshFrameBuffer;

        int mNumberOfClusters = 0;
        glm::ivec3 mClusterGrid;

        DXHeapHandle mDirectionalLightsSRVSlot;
        DXHeapHandle mPointLightsSRVSlot;
        DXHeapHandle mCompactClusterSRVSlot;
        DXHeapHandle mClusterSRVSlot;
        DXHeapHandle mActiveClusterSRVSlot;
        DXHeapHandle mLightIndicesSRVSlot;
        DXHeapHandle mLightGridSRVSlot;

        DXHeapHandle mClusterUAVSlot;
        DXHeapHandle mActiveClusterUAVSlot;
        DXHeapHandle mCompactClusterUAVSlot;
        DXHeapHandle mLightGridUAVSlot;
        DXHeapHandle mPointLightCounterUAVSlot;
        DXHeapHandle mLightIndicesUAVSlot;

        DebugRenderingData mDebugRenderingData;
        UIRenderingData mUIRenderingData;
        PosProcRenderingData mPostProcData;
	};
}

