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
        void CalculateClusterGrid(const GPUWorld& gpuWorld);
        void CullClusters(const World& world, const GPUWorld& gpuWorld);
        void CompactClusters(const GPUWorld& gpuWorld);
        void AssignLights(const GPUWorld& gpuWorld, int numberOfCompactClusters);
        //void CullClustersCS(const GPUWorld& gpuWorld, const FrameBuffer& buffer);

        void ClusteredShading(const World& world);

        std::unique_ptr<DXPipeline> mPBRPipeline;
        std::unique_ptr<DXPipeline> mPBRSkinnedPipeline;
        std::unique_ptr<DXPipeline> mClusterGridPipeline;
        std::unique_ptr<DXPipeline> mCullClusterPipeline;
        std::unique_ptr<DXPipeline> mCullClusterSkinnedMeshPipeline;
        std::unique_ptr<DXPipeline> mCompactClusterPipeline;
        std::unique_ptr<DXPipeline> mAssignLigthsPipeline;
        std::unique_ptr<DXPipeline> mZPipeline;
        std::unique_ptr<DXPipeline> mZSkinnedPipeline;
    };
}