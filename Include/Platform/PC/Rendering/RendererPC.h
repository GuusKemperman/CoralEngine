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
            DIRECTIONAL_LIGHT_SB,
            POINT_LIGHT_SB,
            CLUSTER_GRID_SB,
            ACTIVE_CLUSTER_SB,
            COMPACT_CLUSTER_SB,
            CLUSTER_COUNTER_BUFFER,
            NUM_SB
        };

        enum DXPipelines {
            PBR_PIPELINE,
            CLUSTER_GRID_PIPELINE,
            CULL_CLUSTER_PIPELINE,
            COMPACT_CLUSTER_PIPELINE,
            ASSIGN_LIGHTS_PIPELINE,
            NUM_PIPELINES
        };

        enum DXConstantBuffers {
            CAM_MATRIX_CB,
            LIGHT_CB,
            MODEL_INDEX_CB,
            MODEL_MATRIX_CB,
            CLUSTER_INFO_CB,
            CLUSTERING_CAM_CB,
            NUM_CBS
        };


    private:
        void CalculateClusterGrid(const CameraComponent& camera);
        void CullClusters(const World& world);
        void CompactClusters();
        void UpdateLights(int numDirLights, int numPointLights);

        std::unique_ptr<DXConstBuffer> mConstBuffers[NUM_CBS];
        std::unique_ptr<DXResource> mStructuredBuffers[NUM_SB];
        std::unique_ptr<DXPipeline> mPipelines[NUM_PIPELINES];

        unsigned int mClusterUAVIndex = 0;
        unsigned int mClusterSRVIndex = 0;
        unsigned int mCompactClusterUAVIndex = 0;
        unsigned int mCompactClusterSRVIndex = 0;
        unsigned int mActiveClusterUAVIndex = 0;
        unsigned int mActiveClusterSRVIndex = 0;
        unsigned int mDirectionalLightsSRVIndex = 0;
        unsigned int mPointLightSRVIndex = 0;

        glm::vec2 screenSize = glm::vec2(1.f, 1.f);
        bool updateClusterGrid = false;
        int mNumberOfTiles = 0;
        
        unsigned int materialHeapSlot = 0;
        std::vector<InfoStruct::DXMaterialInfo> mMaterialVec;
        std::vector<InfoStruct::DXDirLightInfo> mDirectionalLights;
        std::vector<InfoStruct::DXPointLightInfo> mPointLights;
        InfoStruct::DXLightInfo mLightInfo;


    private:
        friend ReflectAccess;
        static MetaType Reflect();
    };
}