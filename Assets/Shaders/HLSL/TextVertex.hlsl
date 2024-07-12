struct VS_INPUT
{
    float4 position : POSITION;
    float4 color : COLOR;
    float2 uv : UV_COORDS;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV_COORDS;
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
    output.position = mul(float4(input.position.xyz, 1.f), pMat);
    output.color = input.color;
    output.uv = input.uv;
   
    return output;
}