#include "Structures.hlsli"

struct VS_INPUT
{
    float3 position : POSITION;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 viewPos : VIEW_POSITION;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    
    float4x4 cameraMat = mul(mView, mProjection);
    
    float4 positionVS = mul(float4(input.position.xyz, 1.0f), mModelMat);
    output.position = mul(positionVS, cameraMat);
    output.viewPos = mul(positionVS, mView);
    
    return output;
}