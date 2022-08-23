#ifndef PCF_HLSLI
#define PCF_HLSLI


static float sampleShadowMapPCF(float4x4 vp, float3 worldPosition, Texture2D<float> shadowMap, float4 viewport,
                                SamplerComparisonState shadowMapSampler, float2 texelSize, float bias, float pcfRadius = 1.5f,
                                float numPCFSamples = 16.f)
{
    float4 lightProjected = mul(vp, float4(worldPosition, 1.f));
    lightProjected.xyz /= lightProjected.w;
    float2 lightUV = lightProjected.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    // (-1 ,1) => (0 ,1), flip y axis due to screen from top to down;
    float visibility = 1.f;
    if(all(lightUV >= 0.f && lightUV <= 1.f)) {
        lightUV = lightUV * viewport.zw + viewport.xy;
        visibility = 0.f;
        for(float y = -pcfRadius; y <= pcfRadius + 0.01f; y += 1.f) {
            for(float x = -pcfRadius; x <= pcfRadius + 0.01f; x += 1.f) {
                visibility += shadowMap.SampleCmpLevelZero(shadowMapSampler, lightUV + float2(x, y) * texelSize,
                                                           lightProjected.z - bias);
            }
        }
        visibility /= numPCFSamples;
    }
    return visibility;
}

static float samplePointLightShadowMapPCF(float3 worldPosition, float3 lightPosition, Texture2D<float> shadowMap,
                                          float4 viewport1, float4 viewport2, SamplerComparisonState shadowMapSampler,
                                          float2 texelSize, float maxDistance, float pcfRadius = 1.5f,
                                          float numPCFSamples = 16.f)
{
    float3 L = worldPosition - lightPosition;
    float len = length(L);
    L /= len;
    float flip = L.z > 0 ? 1.f : -1.f;
    float4 viewport = L.z > 0.f ? viewport1 : viewport2;
    L.z *= flip;
    L.xy /= L.z+1.f;        //unpack

    float2 lightUV = L.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    lightUV = lightUV * viewport.zw + viewport.xy;
    float compareDistance = len / maxDistance;
    float bias = 0.001f * flip;
    float visibility = 0.f;
    for (float y = -pcfRadius; y <= pcfRadius + 0.01f; y += 1.f)
    {
        for (float x = -pcfRadius; x <= pcfRadius + 0.01f; x += 1.f)
        {
            visibility += shadowMap.SampleCmpLevelZero(
                shadowMapSampler,
                lightUV + float2(x, y) * texelSize,
                compareDistance + bias);
        }
    }
    visibility /= numPCFSamples;
    return visibility;
}


#endif