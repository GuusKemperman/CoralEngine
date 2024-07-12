struct VS_INPUT
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
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
    float4x4 mModelMat;
    float4x4 mInvTransposeMat;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = mul(float4(input.pos.xyz, 1.f), mModelMat);
    output.pos = mul(output.pos, pMat);
    output.uv = input.uv;
    return output;
}