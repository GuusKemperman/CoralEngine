#include "Functions.hlsli"

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 viewPos : VIEW_POSITION;
};

RWStructuredBuffer<bool> RWActiveClusters : register(u1);

[earlydepthstencil]
float4 main(PS_INPUT input) : SV_TARGET
{
    uint clusterIndex = GetClusterIndex(input.position.xy, input.viewPos.z);
    RWActiveClusters[clusterIndex] = true;
    
    return float4(0.f, 0.f, 0.f, 1.f);
}