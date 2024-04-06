#pragma once
#include "Rendering/ISubRenderer.h"
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"

class DXPipelineBuilder;

namespace CE
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
        void DepthPrePass(const World& world, const GPUWorld& gpuWorld);
        void HandleColorComponent(const World& world, const entt::entity& entity, int meshCounter, int frameIndex);
        void RenderShadowMapsStaticMesh(const World& world, const GPUWorld& gpuWorld);
        //TODO: RenderShadowMapsSkinnedMesh
        void CalculateClusterGrid(const GPUWorld& gpuWorld);
        void CullClusters(const World& world, const GPUWorld& gpuWorld);
        void CompactClusters(const GPUWorld& gpuWorld);
        void AssignLights(const GPUWorld& gpuWorld, int numberOfCompactClusters);
        void ClusteredShading(const World& world);

        ComPtr<ID3D12PipelineState>  mPBRPipeline;
        ComPtr<ID3D12PipelineState>  mPBRSkinnedPipeline;
        ComPtr<ID3D12PipelineState>  mClusterGridPipeline;
        ComPtr<ID3D12PipelineState>  mCullClusterPipeline;
        ComPtr<ID3D12PipelineState>  mCullClusterSkinnedMeshPipeline;
        ComPtr<ID3D12PipelineState>  mCompactClusterPipeline;
        ComPtr<ID3D12PipelineState>  mAssignLigthsPipeline;
        ComPtr<ID3D12PipelineState>  mZPipeline;
        ComPtr<ID3D12PipelineState>  mZSkinnedPipeline;
    };
}