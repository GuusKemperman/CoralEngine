#define MAX_BONES 41
#define MAX_BONE_INFLUENCE 4

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 norm : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    int4 boneIds : BONEIDS;
    float4 boneWeights : BONEWEIGHTS;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 vertexPos : POSITION;
    float4 viewPos : VIEW_POSITION;
    float4 norm : NORMAL;
    float4 fragPosLightSpace : LIGHTSPACE_POS;
    float2 texCoord : TEXCOORD;
    float3x3 tangentBasis : TBASIS;
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

struct DirLight
{
    float4 dir;
    float4 colorAndIntensity;
    float4x4 lightSpaceMatrix;
    bool mCastsShadows;
    uint mNumSamples;
    float mBias;
    uint padding;
};

struct PointLight
{
    float4 pos;
    float4 colorAndInt;
    float radius;
};

cbuffer PointLightBuffer : register(b3)
{
    uint numDirLight;
    uint numPointLight;
    int activeShadowingLight;
    uint padding;
}

StructuredBuffer<DirLight> dirLights : register(t5);

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    float4 totalPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float3 totalNormal = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        if (input.boneIds[i] == -1)
            continue;
        if (input.boneIds[i] >= MAX_BONES)
        {
            totalPosition = float4(input.pos.xyz, 1.0f);
            totalNormal = float3(input.norm.xyz);
            break;
        }
        float4 localPosition = mul(finalBoneMatrices[input.boneIds[i]], float4(input.pos.xyz, 1.0));
        totalPosition += localPosition * input.boneWeights[i];
        float3 localNormal = mul((float3x3) finalBoneMatrices[input.boneIds[i]], input.norm.xyz);
        totalNormal += localNormal * input.boneWeights[i];
    }
    
    float4x4 cameraMat = mul(vMat, pMat);
    
    output.pos = float4(totalPosition);
    output.vertexPos = mul(output.pos, modelMat);
    output.viewPos = mul(output.vertexPos, vMat);
    output.pos = mul(output.vertexPos, cameraMat);
    output.norm = float4(normalize(totalNormal), 0.f);

    output.norm = float4(normalize(mul(totalNormal, (float3x3) invTransposeMat)), 0.f);
    output.texCoord = input.texCoord;
    
    float3 bitangent = cross(totalNormal, input.tangent);
    float3x3 TBN = float3x3(input.tangent, bitangent, normalize(totalNormal));
    output.tangentBasis = mul(TBN, (float3x3) invTransposeMat);
    
    //TODO: Implement this
    output.fragPosLightSpace = mul(output.vertexPos, dirLights[activeShadowingLight].lightSpaceMatrix);
    
    return output;
}
