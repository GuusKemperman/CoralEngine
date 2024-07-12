struct VS_INPUT
{
    float3 pos : POSITION;
    float3 norm : NORMAL;
    float3 tangent : TANGENT;
    float2 texCoord : TEXCOORD;
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
    
    float4x4 cameraMat = mul(vMat, pMat);
    
    output.pos = float4(input.pos.xyz, 1.0f);
    output.vertexPos = mul(output.pos, modelMat);
    output.viewPos = mul(output.vertexPos, vMat);
    output.pos = mul(output.vertexPos, cameraMat);
    output.norm = float4(normalize(input.norm.xyz), 0.f);

    output.norm = float4(normalize(mul(input.norm.xyz, (float3x3) invTransposeMat)), 0.f);
    output.texCoord = input.texCoord;
    
    input.tangent = normalize(input.tangent);
    input.tangent = normalize(input.tangent - dot(input.tangent, input.norm) * input.norm);
    float3 bitangent = cross(output.norm.xyz, input.tangent);
    
    float3x3 TBN = float3x3(input.tangent, bitangent, output.norm.xyz);
    
    output.tangentBasis = TBN;
    output.fragPosLightSpace = mul(output.vertexPos, dirLights[activeShadowingLight].lightSpaceMatrix);
    
    return output;
}
