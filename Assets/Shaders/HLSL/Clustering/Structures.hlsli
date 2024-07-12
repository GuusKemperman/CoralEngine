#ifndef __STRUCTURES_INCLUDE_HLSL__
#define __STRUCTURES_INCLUDE_HLSL__

static const uint sNumThreads = 1024;

// A cluster volume is represented using an AABB
struct AABB
{
    float4 mMin;
    float4 mMax;
};

struct Sphere
{
    float3 mPosition;
    float mRadius;
};

struct LightGridElement
{
    uint mOffset;
    uint mCount;
    uint offset[2];
};

struct PointLight
{
    float4 mPosition;
    float4 mColorIntensity;
    float mRadius;
    float3 padding;
};

struct DirLight
{
    float4 mDirection;
    float4 mColorIntensity;
};

cbuffer ClusterBuffer : register(b0)
{
    uint mNumClustersX;
    uint mNumClustersY;
    uint mNumClustersZ;
    uint mMaxLightsInCluster;
}

cbuffer CameraBuffer : register(b1)
{
    float4x4 mView;
    float4x4 mProjection;
    float4x4 mInvView;
    float4x4 mInvProjection;
};

cbuffer CameraClusteringBuffer : register(b2)
{
    float mNearPlane;
    float mFarPlane;

    float2 mLinearDepthCoefficient;
    float2 mScreenDimensions;
    float2 mTileSize;

    float mDepthSliceScale;
    float mDepthSliceBias;
};

cbuffer LightBuffer : register(b3)
{
    uint mNumDirectionalLights;
    uint mNumPointLights;
    uint2 padding; 
}

cbuffer ModelMatrix : register(b4)
{
    float4x4 mModelMat;
    float4x4 mInvModelMat;
};

StructuredBuffer<DirLight> dirLights : register(t2);
StructuredBuffer<PointLight> pointLights : register(t3);

#endif // __STRUCTURES_INCLUDE_HLSL__