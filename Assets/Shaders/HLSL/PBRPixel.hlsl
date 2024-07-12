static const float sPi = 3.14159265359;
// TODO: Should be a setting, since this makes the prospero and pc builds somewhat 
// different due to prospero doing their own internal gamma correction
static const float sGamma = 1.8; 
static const float sInvGamma = 1.0 / sGamma;

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
    float3 padding;
};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 vMat;
    float4x4 pMat;
    float4x4 invVMAT;
    float4x4 invPMAT;
};

cbuffer PointLightBuffer : register(b3)
{
    uint numDirLight;
    uint numPointLight;
    int activeShadowingLight;
    uint padding;
    float4 ambientLightIntensity;
}

cbuffer MaterialInfo : register(b4)
{
    float4 colorFactor;
    float4 emissiveFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    uint useColorTex;
    uint useEmissiveTex;
    uint useMetallicRoughnessTex;
    uint useNormalTex;
    uint useOcclusionTex;
    float4 uvScale;
}

cbuffer ColorMult : register(b5)
{
    float4 colorMul;
    float4 colorAdd;
};

cbuffer CameraClusteringBuffer : register(b6)
{
    float mNearPlane;
    float mFarPlane;

    float2 mLinearDepthCoefficient;
    float2 mScreenDimensions;
    float2 mTileSize;

    float mDepthSliceScale;
    float mDepthSliceBias;
};

cbuffer ClusterBuffer : register(b7)
{
    uint mNumClustersX;
    uint mNumClustersY;
    uint mNumClustersZ;
    uint mMaxLightsInCluster;
}
    
cbuffer FogBuffer : register(b8)
{
    float4 mColor;
    float mFogNearPlane;
    float mFogFarPlane;
    uint mApplyFog;
    uint padding2;
}

struct PBRMaterial
{
    float3 baseColor;
    float3 emissiveColor;
    float metallic;
    float roughness;
    float3 normalColor;
    float occlusionColor;
    float3 F0;
    float3 diffuse;
};

struct LightGridElement
{
    uint mOffset;
    uint mCount;
    uint padding[2];
};


SamplerState mainSampler : register(s0);
SamplerComparisonState depthSampler : register(s1);

Texture2D baseColorTex : register(t0);
Texture2D emissiveTex : register(t1);
Texture2D metallicRoughnessTex : register(t2);
Texture2D normalTex : register(t3);
Texture2D occlusionTex : register(t4);
Texture2D shadowMap : register(t9);

StructuredBuffer<DirLight> dirLights : register(t5);
StructuredBuffer<PointLight> pointLights : register(t6);

StructuredBuffer<LightGridElement> LightGrid : register(t7);
StructuredBuffer<int> LightIndices : register(t8);


PBRMaterial GenerateMaterial(VS_OUTPUT input);
float3 LinearToSRGB(float3 color);
float Attenuation(float distance, float range);
float3 LambertianDiffuse(float3 albedo, float3 f0, float3 f90, float vDotH);
float3 FSchlick(float3 F0, float3 F90, float vDotH);
float D_GGX(in float NdotH, in float alphaRoughness);
float V_GGX(in float NdotL, in float NoV, in float alphaRoughness);
void GetBRDF(PBRMaterial mat, float3 viewDir, float3 lightDir, float3 lightColor, float lightIntensity, float attenuation, inout float3 diffuse, inout float3 specular);
float3 ApplyFog(float3 pixelColor, float distance);

uint GetClusterIndex(float2 pixelPosition, float linearDepth);
float ShadowCalculation(float4 fragPosLightSpace, float3 lightDir, float bias, int numSamples);

float4 main(VS_OUTPUT input) : SV_TARGET
{
    PBRMaterial mat = GenerateMaterial(input);
    float4 cameraPosition = { invVMAT[3][0], invVMAT[3][1], invVMAT[3][2], 0.0 };
    float3 viewDirection = normalize(cameraPosition.xyz - input.vertexPos.xyz);
    
    float3 diffuse = float3(0.0, 0.0, 0.0);
    float3 specular = float3(0.0, 0.0, 0.0);
    
    for (uint i = 0; i < numDirLight; i++)
    {
        float3 lightDirection = -normalize(dirLights[i].dir).xyz;
        float att = 1.0; // Directional light doesn't have attenuation
        
        float3 dif = float3(0.0, 0.0, 0.0);
        float3 spc = float3(0.0, 0.0, 0.0);
        
        GetBRDF(mat, viewDirection, lightDirection, dirLights[i].colorAndIntensity.rgb, dirLights[i].colorAndIntensity.a, att, dif, spc);
        float shadow = 1;

        if (i == activeShadowingLight)
            shadow *= ShadowCalculation(input.fragPosLightSpace, dirLights[i].dir.xyz, dirLights[i].mBias, dirLights[i].mNumSamples);
        
        diffuse += dif *shadow;
        specular += spc * shadow;

    }
    
    uint clusterIndex = GetClusterIndex(input.pos.xy, input.viewPos.z);

    int numLights = LightGrid[clusterIndex].mCount;
    int lightStart = LightGrid[clusterIndex].mOffset;
    
    for (int j = lightStart; j < lightStart + numLights; j++)
    {
        int lightIndex = LightIndices[j];
        if(lightIndex == -1)
            continue;
        PointLight light = pointLights[lightIndex];
        
        float3 lightDirection = light.pos.xyz - input.vertexPos.xyz;
        float dist = length(lightDirection);
        lightDirection /= dist;
        float att = Attenuation(dist, light.radius);
        
        float3 dif = float3(0.0, 0.0, 0.0);
        float3 spc = float3(0.0, 0.0, 0.0);
        
        GetBRDF(mat, viewDirection, lightDirection, light.colorAndInt.rgb, light.colorAndInt.a, att, dif, spc);
        
        diffuse += dif;
        specular += spc;
    }
    
    //AMBIENT LIGHT
    float3 dif = float3(0.0, 0.0, 0.0);
    float3 spc = float3(0.0, 0.0, 0.0);
    float3 lightDirection = cameraPosition.xyz - input.vertexPos.xyz;
    GetBRDF(mat, viewDirection, lightDirection, ambientLightIntensity.rgb, ambientLightIntensity.a, 1.f, dif, spc);
    diffuse += dif;
    specular += spc;


    float3 result = (diffuse + specular) * mat.occlusionColor + mat.emissiveColor;
    result *= colorMul;
    result += colorAdd;
    result = LinearToSRGB(result);
    
    if(mApplyFog)
    {
        float3 view = cameraPosition - input.vertexPos;
        result = ApplyFog(result, length(view));
    }
    return float4(result.rgb, 1.0);
}

PBRMaterial GenerateMaterial(VS_OUTPUT input)
{
    PBRMaterial mat;    
    input.texCoord *= uvScale.xy;
    if (useColorTex)
    {
        mat.baseColor = pow(abs(baseColorTex.Sample(mainSampler, input.texCoord).rgb), sGamma);
        mat.baseColor *= colorFactor.rgb;
    }
    else
    {
        mat.baseColor = float3(1.0, 1.0, 1.0);
        mat.baseColor *= colorFactor.rgb;
    }
        
    if (useEmissiveTex)
    {
        mat.emissiveColor = pow(abs(emissiveTex.Sample(mainSampler, input.texCoord).rgb), sGamma);
        mat.emissiveColor *= emissiveFactor.rgb;
    }
    else
    {
        mat.emissiveColor = float3(0.0, 0.0, 0.0);
    }

    if (useMetallicRoughnessTex)
    {
        float3 metallicRoughnessColor = metallicRoughnessTex.Sample(mainSampler, input.texCoord).rgb;
        mat.roughness = metallicRoughnessColor.g * roughnessFactor;
        mat.metallic = metallicRoughnessColor.b * metallicFactor;
    }
    else
    {
        mat.occlusionColor = 1.0;
        mat.metallic = metallicFactor;
        mat.roughness = roughnessFactor;
    }

    // Occlusion if it is not in matallic roughness texture
    if (useOcclusionTex)
    {
        mat.occlusionColor = occlusionTex.Sample(mainSampler, input.texCoord).r;
    }
    
    if (useNormalTex)
    {
        mat.normalColor = normalTex.Sample(mainSampler, input.texCoord).rgb * normalScale;
        mat.normalColor = mat.normalColor * 2.0 - 1.0;
        mat.normalColor = mul(mat.normalColor, input.tangentBasis);
    }
    else
    {
        mat.normalColor = input.norm.xyz;
    }

    mat.F0 = float3(0.04, 0.04, 0.04);
    mat.F0 = lerp(mat.F0, mat.baseColor, mat.metallic);
    mat.diffuse = lerp(mat.baseColor, float3(0.0, 0.0, 0.0), mat.metallic);
    
    // To alpha roughness
    mat.roughness = mat.roughness * mat.roughness;
    
    
    return mat;
}

// linear to sRGB approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
float3 LinearToSRGB(float3 color)
{
    return pow(color, float3(sInvGamma, sInvGamma, sInvGamma));
}

float Attenuation(float distance, float range)
{
    // GLTF 2.0 uses quadratic attenuation, but with range
    float distance2 = distance * distance;
    return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / distance2;
}

float3 LambertianDiffuse(float3 albedo, float3 f0, float3 f90, float vDotH)
{
    return (1.0 - FSchlick(f0, f90, vDotH)) * (albedo / sPi);
}

float3 FSchlick(float3 F0, float3 F90, float vDotH)
{
    return F0 + (F90 - F0) * pow(clamp(1.0 - vDotH, 0.0, 1.0), 5.0);
}

float D_GGX(in float NdotH, in float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float f = (NdotH * NdotH) * (alphaRoughnessSq - 1.0) + 1.0;

    return alphaRoughnessSq / (sPi * f * f);
}

float V_GGX(in float NdotL, in float NoV, in float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;

    float GGXV = NdotL * sqrt(NoV * NoV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NoV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }

    return 0.0;
}

void GetBRDF(
    PBRMaterial mat, 
    float3 viewDir, 
    float3 lightDir, 
    float3 lightColor, 
    float lightIntensity,
    float attenuation,
    inout float3 diffuse, 
    inout float3 specular)
{
    float3 halfAngle = normalize(viewDir + lightDir);
    float nDotL = clamp(dot(mat.normalColor, lightDir), 0.0, 1.0);
    float nDotH = clamp(dot(mat.normalColor, halfAngle), 0.0, 1.0);
    float nDotV = clamp(dot(mat.normalColor, viewDir), 0.0, 1.0);
    float vDotH = clamp(dot(viewDir, halfAngle), 0.0, 1.0);
    
    float3 colorIntensity = lightColor * lightIntensity;
    colorIntensity *= attenuation;
    
    float3 diffuseBRDF = LambertianDiffuse(mat.diffuse, mat.F0, float3(1.0, 1.0, 1.0), vDotH);

    float3 F = FSchlick(mat.F0, float3(1.0, 1.0, 1.0), vDotH);
    float3 G = V_GGX(nDotL, nDotV, mat.roughness);
    float3 D = D_GGX(nDotH, mat.roughness);
    float3 specularBRDF = F * G * D;
    
    diffuse += colorIntensity * nDotL * diffuseBRDF;
    specular += colorIntensity * nDotL * specularBRDF;
}

float ShadowCalculation(float4 fragPosLightSpace, float3 lightDir, float bias, int numSamples)
{
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    float currentDepth = projCoords.z;
    currentDepth -= bias;
    
    uint width, height, numMips;
    shadowMap.GetDimensions(0, width, height, numMips);

    // Texel size.
    float dx = 1.0f / (float) width;

    float percentLit = 0.0f;
    uint sampleCount = 0;

    for (int x = -numSamples; x <= numSamples; ++x)
    {
        for (int y = -numSamples; y <= numSamples; ++y)
        {
            float2 uv = projCoords.xy + int2(x, y) * dx;
            float shadow= shadowMap.SampleCmpLevelZero(depthSampler, uv, currentDepth).r;
            percentLit += projCoords.z > shadow ? 0.0 : 1.0;
            sampleCount++;
            
        }
    }

    return percentLit / sampleCount;
}

//TODO: Restructure this function so it can be used from Functions.hlsl
uint GetClusterIndex(float2 pixelPosition, float linearDepth)
{
    uint depthSlice = uint(max(log2(linearDepth) * mDepthSliceScale + mDepthSliceBias, 0.0f));

    
    uint3 clusterIndex3D = uint3(pixelPosition / mTileSize, depthSlice);
    uint clusterIndex = clusterIndex3D.x +
                            clusterIndex3D.y * mNumClustersX +
                            clusterIndex3D.z * (mNumClustersX * mNumClustersY);

    return clusterIndex;
}

float3 ApplyFog(float3 pixelColor, float distance)
{
    float fogAmount = saturate((distance - mFogNearPlane) / (mFogFarPlane - mFogNearPlane));
    float3 pixelColor2 = lerp(pixelColor, mColor.rgb, fogAmount);
    return pixelColor2;
}
