#pragma once

#include "DX12Classes/DXDefines.h"
#include "glm/glm.hpp"

namespace Engine::InfoStruct
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
        float padding1; // Padding to align the next bools

        // Grouping bools together for efficient packing, and adding padding to ensure
        // the struct ends on a 16-byte boundary. Even though individual bools are treated
        // as 4 bytes in HLSL, aligning them like this without additional padding will not align
        // the struct size since the total size becomes 60 bytes, which is not a multiple of 16.
        bool useColorTex;
        bool useEmissiveTex;
        bool useMetallicRoughnessTex;
        bool useNormalTex;
        bool useOcclusionTex;
        char padding2[11]; // Additional padding to align the struct size to a multiple of 16 bytes        
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
            uint32 mOffset;
            uint32 mCount;
            uint32 offset[2];
        };
    }
}