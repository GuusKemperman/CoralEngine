#pragma once
#include "DX12Classes/DXDefines.h"
#include "Meta/MetaReflect.h"
#include "Systems/System.h"
#include "Platform/PC/Rendering/InfoStruct.h"
#include "glm/glm.hpp"
#include "Assets/StaticMesh.h"

class DXResource;
class DXConstBuffer;
class DXPipeline;
class DXSignature;
class DXDescHeap;

namespace Engine
{
    class World;
    class CameraComponent;

    class Renderer final :
        public System
    {
    public:
        Renderer();
        void Render(const World& world) override;

        SystemStaticTraits GetStaticTraits() const override
        {
            SystemStaticTraits traits{};
            traits.mPriority = static_cast<int>(TickPriorities::Render) + (TickPriorityStepSize >> 1);
            return traits;
        }

        enum DXStructuredBuffers {
            MODEL_MAT_SB,
            MATERIAL_SB,
            CLUSTER_GRID_SB,
            ACTIVE_CLUSTER_SB,
            COMPACT_CLUSTER_SB,
            COUNTER_BUFFER
        };

        enum DXPipelines {
            PBR_PIPELINE,
            CLUSTER_GRID_PIPELINE,
            CULL_CLUSTER_PIPELINE,
            COMPACT_CLUSTER_PIPELINE
        };


    private:
        void CalculateClusterGrid(const CameraComponent& camera);
        void CullClusters(const World& world);
        void CompactClusters();

        std::unique_ptr<DXConstBuffer> mConstBuffers[NUM_CBS];
        std::unique_ptr<DXResource> mStructuredBuffers[6];
        std::unique_ptr<DXPipeline> mPipelines[4];
        InfoStruct::DXLightInfo mLights;

        unsigned int mClusterUAVIndex = 0;
        unsigned int mClusterSRVIndex = 0;
        unsigned int mCompactClusterUAVIndex = 0;
        unsigned int mCompactClusterSRVIndex = 0;
        unsigned int mActiveClusterUAVIndex = 0;
        unsigned int mActiveClusterSRVIndex = 0;
        glm::vec2 screenSize = glm::vec2(1.f, 1.f);
        bool updateClusterGrid = false;
        int mNumberOfTiles = 0;
        
        unsigned int materialHeapSlot = 0;
        std::vector<InfoStruct::DXMaterialInfo> materials;
    private:
        friend ReflectAccess;
        static MetaType Reflect();
    };
}