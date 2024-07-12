struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV_COORDS;
};
static const float sGamma = 1.8;

Texture2D baseColorTex : register(t0);
SamplerState mainSampler : register(s0);

float4 main(PS_INPUT input) : SV_TARGET
{
    return pow(abs(baseColorTex.Sample(mainSampler, input.uv)), sGamma) * input.color;
}