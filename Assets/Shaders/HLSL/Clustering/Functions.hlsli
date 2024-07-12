#include "Structures.hlsli"

#ifndef __FUNCTIONS_INCLUDE_HLSL__
#define __FUNCTIONS_INCLUDE_HLSL__

uint GetClusterIndex(float2 pixelPosition, float linearDepth)
{   
    uint depthSlice = uint(max(log2(linearDepth) * mDepthSliceScale + mDepthSliceBias, 0.0f));

    uint3 clusterIndex3D = uint3(pixelPosition / mTileSize, depthSlice);
    uint clusterIndex = clusterIndex3D.x +
                            clusterIndex3D.y * mNumClustersX +
                            clusterIndex3D.z * (mNumClustersX * mNumClustersY);

    return clusterIndex;

}

uint3 GetClusterIndex3D(float2 pixelPosition, float linearDepth)
{
    uint depthSlice = uint(max(log2(linearDepth) * mDepthSliceScale + mDepthSliceBias, 0.0f));

    uint3 clusterIndex3D = uint3(pixelPosition / mTileSize, depthSlice);
    
    return clusterIndex3D;
}

uint GetNumOfClusters()
{
    return mNumClustersX * mNumClustersY * mNumClustersZ;
}
#endif // __FUNCTIONS_INCLUDE_HLSL__