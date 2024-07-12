struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};
static const float sGamma = 1.8;


Texture2D baseColorTex : register(t0);
SamplerState mainSampler : register(s0);

cbuffer ColorCB : register(b3)
{
    float4 mColor;
    bool useTexture;
    uint3 padding1;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 color;
    if(useTexture)
        color = pow(abs(baseColorTex.SampleLevel(mainSampler, input.uv, 0)), sGamma) * mColor;
    else
        color = mColor;
       
    return color;
}