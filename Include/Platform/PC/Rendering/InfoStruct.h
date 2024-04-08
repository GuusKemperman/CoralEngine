#pragma once

#include "DX12Classes/DXDefines.h"
#include "glm/glm.hpp"

namespace CE::InfoStruct
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
        uint32 numDirLights = 0;
        uint32 numPointLights = 0;
        uint32 padding[2];
    };

    struct DXMaterialInfo
    {
        glm::vec4 colorFactor;
        glm::vec4 emissiveFactor;
        float metallicFactor;
        float roughnessFactor;
        float normalScale;
        uint32 useColorTex;
        uint32 useEmissiveTex;
        uint32 useMetallicRoughnessTex;
        uint32 useNormalTex;
        uint32 useOcclusionTex;
        glm::vec4 uvScale = glm::vec4(1);            
    };

    struct DXColorMultiplierInfo
    {
        glm::vec4 colorMult;
        glm::vec4 colorAdd;
    };

    struct ColorInfo
    {
        glm::vec4 mColor;
        uint32 mUseTexture;
        uint32 mPadding[3];
    };

    enum DXStructuredBuffers {
        MODEL_MAT_SB,
        DIRECTIONAL_LIGHT_SB,
        POINT_LIGHT_SB,
        CLUSTER_GRID_SB,
        ACTIVE_CLUSTER_SB,
        COMPACT_CLUSTER_SB,
        CLUSTER_COUNTER_BUFFER,
        LIGHT_GRID_SB,
        POINT_LIGHT_COUNTER,
        LIGHT_INDICES,
        COMPACT_CLUSTER_READBACK_RESOURCE,
        GRID_READBACK_RESOURCE,
        NUM_SB
    };

    enum DXConstantBuffers {
        CAM_MATRIX_CB,
        LIGHT_CB,
        MATERIAL_INFO_CB,
        MODEL_MATRIX_CB,
        CLUSTER_INFO_CB,
        CLUSTERING_CAM_CB,
        FINAL_BONE_MATRIX_CB,
        COLOR_CB,
        UI_MODEL_MAT_CB,
        FOG_CB,
        NUM_CBS
    };

    struct DXFogInfo {
        glm::vec4 mColor{ 1.0f };
        float mNearPlane = 0.0f;
        float mFarPlane = 500.0f;
        uint32 applyFog = false;
        uint32 padding = {};
    };

    namespace Clustering
    {
        // Number of threads for cluster culling.
        // Each lane processes a 2x2 block.
        static const unsigned int sClusterCullingGroupSize = 8;
        static const unsigned int sClusterCullingBlockSize = sClusterCullingGroupSize * 2;

        // Maximum amount of threads that can run in parallel when assining lights to clusters
        // This limits the amount of lights that one cluster can store.
        static const unsigned int sLightAssignmentThreadCount = 1024;

        struct DXAABB
        {
            glm::vec4 min;
            glm::vec4 max;
        };

        struct DXCluster
        {
            uint32 mNumClustersX;
            uint32 mNumClustersY;
            uint32 mNumClustersZ;
            uint32 mMaxLightsInCluster;
        };

        struct DXCameraClustering
        {
            float mNearPlane;
            float mFarPlane;

            glm::vec2 mLinearDepthCoefficient;
            glm::vec2 mScreenDimensions;
            glm::vec2 mTileSize;

            float mDepthSliceScale;
            float mDepthSliceBias;
        };

        struct DXLightGridElement
        {
            uint32 mOffset = 0;
            uint32 mCount = 0;
            uint32 padding[2] = { 0,0 };
        };
    }
}