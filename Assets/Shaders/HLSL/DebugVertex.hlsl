struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 vMat;
    float4x4 pMat;
    float4x4 invVMAT;
    float4x4 invPMAT;
};

PS_INPUT main(VS_INPUT input)
{
    float4x4 cameraMat = mul(vMat, pMat);
    
    PS_INPUT output;
    output.position = mul(float4(input.position.xyz, 1.0f), cameraMat);
    output.color = input.color;
    
    return output;
}