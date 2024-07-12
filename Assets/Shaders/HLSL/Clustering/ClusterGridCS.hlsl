#include "Structures.hlsli"

RWStructuredBuffer<AABB> RWClustersAABB : register(u0);

float4 ClipToView(float4 clip)
{
    // View space transform
    float4 view = mul(clip, mInvProjection);
    // Perspective projection
    view = view / view.w;
    
    return view;
}

float4 ScreenToView(float4 screen)
{
    // Convert to NDC
    float2 ndc = screen.xy / mScreenDimensions;

    // Convert to clip space
    float4 clip = float4(float2(ndc.x, 1.0f - ndc.y) * 2.0f - 1.0f, screen.z, screen.w);

    return ClipToView(clip);
}

// Finds the intersection of a line segment with a plane
float3 IntersectLineToZPlane(float3 a, float3 b, float z)
{
    // Because this is a Z based normal this is fixed
    const float3 normal = float3(0.0f, 0.0f, 1.0f);

    float3 ab = b - a;

    // Computing the intersection length for the line and the plane
    float t = (z - dot(normal, a)) / dot(normal, ab);

    // Computing the actual xyz position of the point along the line
    return a + t * ab;
}

[numthreads(1, 1, 1)]
void main(const uint3 DTid : SV_DispatchThreadID)
{
    //Eye position is zero in view space
    uint clusterIndex = DTid.x +
                        DTid.y * mNumClustersX +
                        DTid.z * (mNumClustersX * mNumClustersY);
    
    // Calculate the min and max point in screen space
    float4 minPointSS = float4(DTid.xy * mTileSize, 1.0f, 1.0f); // Bottom left
    float4 maxPointSS = float4(float2(DTid.xy + 1) * mTileSize, 1.0f, 1.0f); // Top Right
    
    // Convert points to view space
    float3 minPointVS = ScreenToView(minPointSS).xyz;
    float3 maxPointVS = ScreenToView(maxPointSS).xyz;

    // Depth partition taken from https://www.slideshare.net/TiagoAlexSousa/siggraph2016-the-devil-is-in-the-details-idtech-666
    float planeNear = mNearPlane * pow(mFarPlane / mNearPlane, float(DTid.z) / mNumClustersZ);
    float planeFar = mNearPlane * pow(mFarPlane / mNearPlane, float(DTid.z + 1) / mNumClustersZ);
    
    // Finding the min/max intersection points to the cluster near/far plane
    // Eye position is zero in view space
    const float3 eyePosition = float3(0.f, 0.f, 0.f);
    
    float3 minPointNear = IntersectLineToZPlane(eyePosition, minPointVS, planeNear);
    float3 minPointFar = IntersectLineToZPlane(eyePosition, minPointVS, planeFar);
    float3 maxPointNear = IntersectLineToZPlane(eyePosition, maxPointVS, planeNear);
    float3 maxPointFar = IntersectLineToZPlane(eyePosition, maxPointVS, planeFar);
    
    AABB clusterAABB;
    clusterAABB.mMin = float4(min(min(minPointNear, minPointFar), min(maxPointNear, maxPointFar)), 0.f);
    clusterAABB.mMax = float4(max(max(minPointNear, minPointFar), max(maxPointNear, maxPointFar)), 0.f);

    RWClustersAABB[clusterIndex] = clusterAABB;
}