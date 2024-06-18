#pragma once

#include "DX12Classes/DXDefines.h"
#include "glm/glm.hpp"
#include "Components/Particles/ParticleMeshRendererComponent.h"

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
        glm::mat4x4 mLightMat = {};
        uint32 mCastsShadows = false;
        uint32 mNumSamples = 2;
        float mBias = 0.001f;
        uint32 padding = 0;
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
        int mActiveShadowingLight = -1;
        uint32 padding;
        glm::vec4 mAmbientAndIntensity = glm::vec4(0.f);
    };

    struct DXMaterialInfo
    {
        glm::vec4 colorFactor = glm::vec4(1.f);
        glm::vec4 emissiveFactor =  glm::vec4(1.f);
        float metallicFactor = 0.f;
        float roughnessFactor = 0.f;
        float normalScale = 1.f;
        uint32 useColorTex = 0;
        uint32 useEmissiveTex = 0;
        uint32 useMetallicRoughnessTex = 0;
        uint32 useNormalTex = 0;
        uint32 useOcclusionTex = 0;
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

    struct DXParticleInfo {
        glm::mat4x4 mMatrix = glm::mat4(1.f);
        glm::vec4 mColor = glm::vec4(0.f);
        bool mIsEmissive = false;
        float mDistanceToCamera =0.f;
        float mLightIntensity = 1.f;
        float mLightRadius = 1.f;
        DXMaterialInfo mMaterialInfo{};
        StaticMesh* mMesh;
        Material* mMaterial;
    };

    struct DXFontInfo
    {
        std::unique_ptr<DXResource> mVertexResource;
        std::unique_ptr<DXResource> mIndexResource;
        D3D12_VERTEX_BUFFER_VIEW mVertexResourceView;
        D3D12_INDEX_BUFFER_VIEW mIndexResourceView;
        size_t mIndexCount;
    };

    struct DXFontVert
    {
        glm::vec4 pos;
        glm::vec4 color;
        glm::vec2 uv;
    };


    enum DXStructuredBuffers
    {
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

    enum DXConstantBuffers
    {
        CAM_MATRIX_CB,
        LIGHT_CB,
        MATERIAL_INFO_CB,
        PARTICLE_MATERIAL_INFO_CB,
        MODEL_MATRIX_CB,
        PARTICLE_MODEL_MATRIX_CB,
        CLUSTER_INFO_CB,
        CLUSTERING_CAM_CB,
        FINAL_BONE_MATRIX_CB,
        COLOR_CB,
        PARTICLE_COLOR_CB,
        UI_MODEL_MAT_CB,
        FOG_CB,
        PARTICLE_INFO_CB,
        NUM_CBS
    };

    struct DXShadowMapInfo
    {
        std::unique_ptr<DXResource> mDepthResource;
        std::unique_ptr<DXResource> mRenderTarget;
        DXHeapHandle mDepthHandle;
        DXHeapHandle mDepthSRVHandle;
        DXHeapHandle mRTHandle;

        D3D12_VIEWPORT mViewport;
        D3D12_RECT mScissorRect;
    };
    
    struct DXFogInfo {
        glm::vec4 mColor{ 1.0f };
        float mNearPlane = 0.0f;
        float mFarPlane = 500.0f;
        uint32 mApplyFog = false;
        uint32 padding = {};
    };

    struct DXOutlineInfo
    {
        glm::vec4 mOutlineColor = glm::vec4(1.f);
        float mThicknes = 2.f;
        float mBias = 0.05f;
        float padding[2];
    };

    struct DXParticleBufferInfo
    {
        uint32 mIsEmissive = 0;
        float mEmissionIntensity = 1.f;
        float padding[2];
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