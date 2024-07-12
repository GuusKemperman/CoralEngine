struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D<float4> depthTexture : register(t0);
SamplerState mainSampler : register(s0);

cbuffer ColorCobstBuffer : register(b0)
{
    float4 color;
    float thickness;
    float bias;
    float padding[2];
};


float4 main(PS_INPUT input) : SV_TARGET
{    
    float4 res = float4(0.f, 0.f, 0.f, 0.f);

    uint width, height;
    depthTexture.GetDimensions(width, height);
       
    float2 offset = (1.0 * thickness) / float2(width, height);

    // Sample the depth of the center pixel and its neighbors
    float centerDepth = depthTexture.SampleLevel(mainSampler, input.uv, 0).r;
    float depthTop = depthTexture.SampleLevel(mainSampler, input.uv + float2(0, -offset.y), 0).r;
    float depthBottom = depthTexture.SampleLevel(mainSampler, input.uv + float2(0, offset.y), 0).r;
    float depthLeft = depthTexture.SampleLevel(mainSampler, input.uv + float2(-offset.x, 0), 0).r;
    float depthRight = depthTexture.SampleLevel(mainSampler, input.uv + float2(offset.x, 0), 0).r;

    float edgeStrength = abs(centerDepth - depthTop) + abs(centerDepth - depthBottom) +
                         abs(centerDepth - depthLeft) + abs(centerDepth - depthRight);
    
    if (edgeStrength > bias)
        res = color;
        
    if(res.a == 0.f)
        discard;

    return res;
}