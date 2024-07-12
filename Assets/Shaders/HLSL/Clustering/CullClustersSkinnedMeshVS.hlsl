#include "Structures.hlsli"

struct VS_INPUT
{
    float3 pos : POSITION;
    int4 boneIds : BONEIDS;
    float4 boneWeights : BONEWEIGHTS;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 viewPos : VIEW_POSITION;
};

#define MAX_BONES 128
#define MAX_BONE_INFLUENCE 4

cbuffer FinalBoneMatrices : register(b5)
{
    float4x4 finalBoneMatrices[MAX_BONES];
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    float4 totalPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        if (input.boneIds[i] == -1)
            continue;
        if (input.boneIds[i] >= MAX_BONES)
        {
            totalPosition = float4(input.pos.xyz, 1.0f);
            break;
        }
        float4 localPosition = mul(finalBoneMatrices[input.boneIds[i]], float4(input.pos.xyz, 1.0));
        totalPosition += localPosition * input.boneWeights[i];
    }

    float4x4 cameraMat = mul(mView, mProjection);
    
    float4 positionVS = mul(float4(totalPosition.xyz, 1.0f), mModelMat);
    output.position = mul(positionVS, cameraMat);
    output.viewPos = mul(positionVS, mView);

    return output;
}