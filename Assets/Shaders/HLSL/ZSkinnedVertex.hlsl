#define MAX_BONES 128
#define MAX_BONE_INFLUENCE 4

struct VS_INPUT
{
    float3 pos : POSITION;
    int4 boneIds : BONEIDS;
    float4 boneWeights : BONEWEIGHTS;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 vMat;
    float4x4 pMat;
    float4x4 invVMAT;
    float4x4 invPMAT;
};

cbuffer ModelMatrix : register(b1)
{
    float4x4 modelMat;
    float4x4 invTransposeMat;
};

cbuffer FinalBoneMatrices : register(b2)
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
    
    float4x4 cameraMat = mul(vMat, pMat);
    
    output.pos = totalPosition;
    output.pos = mul(output.pos, modelMat);
    output.pos = mul(output.pos, cameraMat);

    return output;
}