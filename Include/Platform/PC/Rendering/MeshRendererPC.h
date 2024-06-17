#pragma once
#include "Rendering/ISubRenderer.h"
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"

class DXPipelineBuilder;

namespace CE
{
    class World;
    class GPUWorld;
    class Material;

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
        void CalculateClusterGrid(const GPUWorld& gpuWorld);
        void CullClusters(const World& world, const GPUWorld& gpuWorld);
        void CompactClusters(const GPUWorld& gpuWorld);
        void AssignLights(const GPUWorld& gpuWorld, int numberOfCompactClusters);
        void ClusteredShading(const World& world);
        void RenderShadowMaps(const World& world);
        void RenderParticles(const World& world);
        void BindMaterial(const CE::Material& material);

        ComPtr<ID3D12PipelineState>  mPBRPipeline;
        ComPtr<ID3D12PipelineState>  mParticlePBRPipeline;
        ComPtr<ID3D12PipelineState>  mPBRSkinnedPipeline;
        ComPtr<ID3D12PipelineState>  mClusterGridPipeline;
        ComPtr<ID3D12PipelineState>  mCullClusterPipeline;
        ComPtr<ID3D12PipelineState>  mCullClusterParticlePipeline;
        ComPtr<ID3D12PipelineState>  mCullClusterSkinnedMeshPipeline;
        ComPtr<ID3D12PipelineState>  mCompactClusterPipeline;
        ComPtr<ID3D12PipelineState>  mAssignLigthsPipeline;
        ComPtr<ID3D12PipelineState>  mZPipeline;
        ComPtr<ID3D12PipelineState>  mZSkinnedPipeline;
        ComPtr<ID3D12PipelineState>  mShadowMapPipeline;
        ComPtr<ID3D12PipelineState>  mShadowMapSkinnedPipeline;

        ComPtr<ID3D12PipelineState>  mZSelectedPipeline;
        ComPtr<ID3D12PipelineState>  mZSelectedSkinnedPipeline;

    };
}