#pragma once
#include "Rendering/IGPUWorld.h"
#include "DX12Classes/DXDefines.h"
#include "Platform/PC/Rendering/DX12Classes/DXHeapHandle.h"
#include "Platform/PC/Rendering/InfoStruct.h"

class DXResource;
class DXConstBuffer;
class DXDescHeap;

namespace Engine
{
    class Material;
    class CameraComponent;

    class DebugRenderingData
    {
    public:
        DebugRenderingData();
        ~DebugRenderingData();

        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
        std::unique_ptr<DXResource> mVertexBuffer;
        std::unique_ptr<DXConstBuffer> mLineColorBuffer;
        std::unique_ptr<DXConstBuffer> mLineMatrixBuffer;
        std::vector<glm::mat4x4> mModelMats;
        std::vector<glm::vec4> mColors;
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
        void UpdateLights(int numDirLights, int numPointLights);
        void ClearClusterData();

        const DXConstBuffer& GetConstantBuffer(InfoStruct::DXConstantBuffers constantBuffer) const { return *mConstBuffers[constantBuffer]; }
        const DXConstBuffer& GetCameraBuffer() const { return *mConstBuffers[InfoStruct::CAM_MATRIX_CB]; };
        DXConstBuffer& GetModelIndexBuffer() const { return *mConstBuffers[InfoStruct::MODEL_INDEX_CB]; };
        const DXConstBuffer& GetLightBuffer() const { return *mConstBuffers[InfoStruct::LIGHT_CB]; };
        DXConstBuffer& GetModelMatrixBuffer() const { return *mConstBuffers[InfoStruct::MODEL_MATRIX_CB]; };
        DXConstBuffer& GetBoneMatrixBuffer() const { return *mConstBuffers[InfoStruct::FINAL_BONE_MATRIX_CB]; };
        DXConstBuffer& GetMeshColorBuffer() const { return *mConstBuffers[InfoStruct::COLOR_CB]; };

        const InfoStruct::DXMaterialInfo& GetMaterial(int meshIndex) const { return mMaterials[meshIndex]; };

        const DXHeapHandle& GetMaterialHeapSlot() const { return mMaterialHeapSlot; };
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

        DebugRenderingData& GetDebugRenderingData() { return mDebugRenderingData; };
        UIRenderingData& GetUIRenderingData() { return mUIRenderingData; };

        glm::ivec3 GetClusterGrid() const { return mClusterGrid; }
        int GetNumberOfClusters() const { return mNumberOfClusters; }
	private:
        void SendMaterialTexturesToGPU(const Material& mat);
        void UpdateClusterData(const CameraComponent& camera);

		std::unique_ptr<DXConstBuffer> mConstBuffers[InfoStruct::NUM_CBS];
		std::unique_ptr<DXResource> mStructuredBuffers[InfoStruct::NUM_SB];
		InfoStruct::DXLightInfo mLights;
		std::vector<InfoStruct::DXMaterialInfo> mMaterials;
        std::vector<InfoStruct::DXDirLightInfo> mDirectionalLights;
        std::vector<InfoStruct::DXPointLightInfo> mPointLights;
        InfoStruct::DXLightInfo mLightInfo;
        int mNumberOfClusters = 0;
        glm::ivec3 mClusterGrid;

        DXHeapHandle mMaterialHeapSlot;
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
	};
}

