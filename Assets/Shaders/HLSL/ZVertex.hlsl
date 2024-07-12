struct VS_INPUT
{
    float3 pos : POSITION;
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

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    float4x4 cameraMat = mul(vMat, pMat);
    
    output.pos = float4(input.pos.xyz, 1.0f);
    output.pos = mul(output.pos, modelMat);
    output.pos = mul(output.pos, cameraMat);

    return output;
}