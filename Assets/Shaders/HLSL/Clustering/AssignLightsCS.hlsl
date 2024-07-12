#include "Structures.hlsli"

StructuredBuffer<AABB> clustersAABB : register(t0);
StructuredBuffer<uint> compactClusters : register(t1);

RWStructuredBuffer<uint> RWPointLightIndexCounter : register(u0);
RWStructuredBuffer<LightGridElement> RWLightGrid : register(u1);
RWStructuredBuffer<uint> RWLightIndices : register(u2);

groupshared uint gsClusterIndex1D;
groupshared AABB gsClusterAABB;

groupshared uint gsPointLightCount;
groupshared uint  gsPointLightStartOffset;
groupshared uint gsPointLightIndexList[sNumThreads];

float SQDistancePointAABB(float3 p, AABB b)
{
    float sqDistance = 0.0f;

    for (int i = 0; i < 3; ++i)
    {
        float v = p[i];

        if (v < b.mMin[i])
            sqDistance += pow(b.mMin[i] - v, 2);
        if (v > b.mMax[i])
            sqDistance += pow(v - b.mMax[i], 2);
    }

    return sqDistance;
}

bool IntersectSphereAABB(Sphere sphere, AABB aabb)
{
    float squaredDistance = SQDistancePointAABB(sphere.mPosition, aabb);

    return squaredDistance <= (sphere.mRadius * sphere.mRadius);
}

[numthreads(sNumThreads, 1, 1)]
void main(const uint3 groupID : SV_GroupID, const uint threadIndex : SV_GroupIndex)
{
    if (threadIndex == 0)
    {
        gsPointLightCount = 0;
        gsPointLightStartOffset = 0;
        
        gsClusterIndex1D = compactClusters[groupID.x];
        gsClusterAABB = clustersAABB[gsClusterIndex1D];
    }

    // We stall all threads in the group until we know the first thread has reset the values
    GroupMemoryBarrierWithGroupSync();
    
    // Cull point lights
    for (uint i = threadIndex; i < mNumPointLights; i += sNumThreads)
    {
        PointLight pointLight = pointLights[i];
        float3 lightPositionVS = mul(pointLight.mPosition, mView).xyz;
        Sphere sphere = { lightPositionVS, pointLight.mRadius };

        if (IntersectSphereAABB(sphere, gsClusterAABB))
        {
            uint index;
            InterlockedAdd(gsPointLightCount, 1, index);
            
            if (index < mMaxLightsInCluster)
            {
                gsPointLightIndexList[index] = i;
            }
            else
            {
                break;
            }
        }
    }
    
    // Wait for all lights to be checked and added to the light list before adding them to the light grid
    GroupMemoryBarrierWithGroupSync();
    
    // The first thread in the group updates the light grids
    if (threadIndex == 0)
    {
        gsPointLightCount = min(gsPointLightCount, mMaxLightsInCluster);
        
        InterlockedAdd(RWPointLightIndexCounter[0], gsPointLightCount, gsPointLightStartOffset);
        
        // Add information for how to access light indices into the light grid for the current cluster
        RWLightGrid[gsClusterIndex1D].mOffset = gsPointLightStartOffset;
        RWLightGrid[gsClusterIndex1D].mCount = gsPointLightCount;
    }

    // Wait for the first thread in the group to update atomic counter for lights
    GroupMemoryBarrierWithGroupSync();
    
    // Add light indices to light indices list
    for (uint j = threadIndex; j < gsPointLightCount; j += sNumThreads)
    {
        RWLightIndices[gsPointLightStartOffset + j] = gsPointLightIndexList[j];
    }
}