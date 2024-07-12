struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D<float4> floatTexture : register(t0);
Texture2D<float4> lutTexture : register(t1);
SamplerState mainSampler : register(s0);

cbuffer ConstantBuffer : register(b10)
{
    float mExposure;
    float mColorCorrection;
    float mInvertLut;
    float mNumberOfBlocksX;
    float mNumberOfBlocksY;
};

static const float3x3 ACESInputMat =
{
    { 0.59719, 0.35458, 0.04823 },
    { 0.07600, 0.90834, 0.01566 },
    { 0.02840, 0.13383, 0.83777 }
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367 },
    { -0.10208, 1.10813, -0.00605 },
    { -0.00327, -0.07276, 1.07602 }
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 ACESFitted(float3 color)
{
    color = mul(ACESInputMat, color);

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = mul(ACESOutputMat, color);

    // Clamp to [0, 1]
    color = saturate(color);

    return color;
}

float3 ApplyLUT(float3 color);

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 hdr = floatTexture.SampleLevel(mainSampler, input.uv, 0).rgb;
    float3 ldrColor;

    if(mExposure >0)
    {
        ldrColor = ACESFitted(hdr * mExposure);
    }
    else
    {
        ldrColor = saturate(hdr);
    }
  
    // Apply the ACES tone mapping curve
    if(mColorCorrection)
    {      
       ldrColor = ApplyLUT(ldrColor);
    }

    // Output the tone-mapped color
    return float4(ldrColor, 1.0);
}

float3 ApplyLUT(float3 color)
{
    
    float3 LUTDimentions;
    lutTexture.GetDimensions(0, LUTDimentions.x, LUTDimentions.y, LUTDimentions.z);
    
    float colors = mNumberOfBlocksX;
    float maxColor = colors - 1.0;
    float halfColX = 0.5 / LUTDimentions.x;
    float halfColY = 0.5 / LUTDimentions.y;
    float threshold = maxColor / colors;
    
    float xOffset = halfColX + color.r * threshold / colors;
    float yOffset = halfColY + color.g * threshold;
    
// Determine the two slices to sample from
    float cell = color.b * maxColor;
    float cell0 = floor(cell);
    float cell1 = min(cell0 + 1.0, maxColor); // Ensure we don’t go out of bounds

// Calculate the positions for both slices
    float2 lutPos0 = float2((cell0 / colors + xOffset), yOffset);
    float2 lutPos1 = float2((cell1 / colors + xOffset), yOffset);
    
    if (mInvertLut == 1)
    {
        lutPos0.y *= -1;
        lutPos1.y *= -1;
    }
    
// Sample the LUT texture at both slices
    float3 color0 = lutTexture.SampleLevel(mainSampler, lutPos0, 0).rgb;
    float3 color1 = lutTexture.SampleLevel(mainSampler, lutPos1, 0).rgb;
    
    // Interpolate between the two colors based on the fractional part of cell
    float blendFactor = frac(cell);
    float3 gradedCol = lerp(color0, color1, blendFactor);
    
    return gradedCol;
}