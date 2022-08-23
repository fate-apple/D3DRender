#include "Brdf.hlsli"
#include "DefaultPbr_rs.hlsli"
#include "Material.hlsli"
#include "Camera.hlsli"
#include "LightProbe.hlsli"
#include "LightSource.hlsli"
#include "normal.hlsli"
#include "LightCulling_rs.hlsli"


struct PsInput
{
    float4 screenPosition : SV_POSITION;
    float3 worldPosition : POSITION;
    float3x3 tbn : TANGENT_FRAME;
    float2 uv : TEXCOORDS;
    bool isFrontFace : SV_IsFrontFace;
};

ConstantBuffer<PbrMaterialCB> material : register(b0, space1);
ConstantBuffer<CameraCB> camera : register(b1, space1);
ConstantBuffer<LightingCB> lighting : register(b2, space1);
ConstantBuffer<LightProbeGridCB> lightProbeGrid : register(b3, space1); // 2d linear, CLAMP

SamplerState wrapSampler : register(s0);
SamplerState clampSampler : register(s1);
SamplerComparisonState shadowSampler : register(s2);

Texture2D<float4> albedoTex : register(t0, space1);
Texture2D<float3> normalTex : register(t1, space1);
Texture2D<float> roughnessTex : register(t2, space1);
Texture2D<float> metallicTex : register(t3, space1);

ConstantBuffer<DirectionalLightCB> sun : register(b0, space2);

TextureCube<float4> irradianceTexture : register(t0, space2);
TextureCube<float4> environmentTexture : register(t1, space2);
Texture2D<float2> brdf : register(t2, space2);
Texture2D<uint4> tiledCullingGrid : register(t3, space2);
StructuredBuffer<uint> tiledObjectsIndexList : register(t4 , space2);
StructuredBuffer<PointLightCB> pointLights : register(t5 , space2);
StructuredBuffer<SpotLightCB> SpotLights : register(t6 , space2);
StructuredBuffer<PbrDecalCB> decals : register(t7 , space2);

Texture2D<float> shadowMap : register(t8, space2);
StructuredBuffer<PointShadowInfo> pointShadowInfos : register(t9 , space2);
StructuredBuffer<SpotShadowInfo> spotShadowInfos : register(t10 , space2);

Texture2D<float4> decalTextureAtlas : register(t11 , space2);
Texture2D<float> aoTexture : register(t12 , space2);
Texture2D<float> sssTexture : register(t13 , space2);
Texture2D<float4> ssrTexture : register(t14 , space2);

Texture2D<float3> lightProbeIrradiance : register(t15, space2);
Texture2D<float2> lightProbeDepth : register(t16, space2);

struct psOutput
{
    float4 hdrColor : SV_Target0;
#ifndef TRANSPARENT
    float4 worldNormalRoughness : SV_Target1;
#endif
};

[earlydepthstencil]
[RootSignature(DEFAULT_PBR_RS)]
psOutput main(PsInput In)
{
    uint flags = material.getFlags();
    float2 materialUV = In.uv * material.uvScale();

    SurfaceInfo surface;
    surface.albedo = (flags & USE_ALBEDO_TEXTURE ? albedoTex.Sample(wrapSampler, materialUV) : float4(1.f, 1.f, 1.f, 1.f)) *
    material.getAlbedo();
    const float normalMapStrength = material.getNormalMapStrength() * 0.2f;
    surface.N = (flags & USE_NORMAL_TEXTURE)
                    ? mul(float3(normalMapStrength, normalMapStrength, 1.f) * SampleNormalMap(
                              normalTex, wrapSampler, materialUV), In.tbn)
                    : In.tbn[2];
    surface.N = normalize(surface.N);
    if(material.doubleSided() && !In.isFrontFace) {
        surface.N = - surface.N;
    }
    surface.roughness = flags & USE_ROUGHNESS_TEXTURE
                            ? roughnessTex.Sample(wrapSampler, materialUV)
                            : material.getRoughnessOverride();
    surface.roughness = clamp(surface.roughness, 0.01f, 0.99f);
    surface.metallic = (flags & USE_METALLIC_TEXTURE)
                           ? metallicTex.Sample(wrapSampler, materialUV)
                           : material.getMetallicOverride();
    surface.emission = material.emission;

    surface.P = In.worldPosition;
    float3 cameraToP = surface.P - camera.position.xyz;
    surface.V = -normalize(cameraToP);
    float pixelDepth = dot(camera.forward.xyz, cameraToP);

    LightContribution totalLighting = {float3(0.f, 0.f, 0.f), float3(0.f, 0.f, 0.f)};
    const uint2 tileIndex = uint2(In.screenPosition.xy / LIGHT_CULLING_TILE_SIZE);
    //recall LightCulling
#ifndef TRANSPARENT
    const uint2 tiledIndexData = tiledCullingGrid[tileIndex].xy;
#else
    const uint2 tiledIndexData = tiledCullingGrid[tileIndex].zw;
#endif
    const uint pointLightCount = (tiledIndexData.y >> 20) & 0x3FF;
    const uint spotLightCount = (tiledIndexData.y >> 10) & 0x3FF;
    const uint decalReadOffset = tiledIndexData.x;
    uint lightReadIndex = decalReadOffset + TILE_LIGHT_OFFSET;

    //  Decals
    float3 decalAlbedoAccum = (float3)0.f;
    float decalRoughnessAccum = 0.f;
    float decalMetallicAccum = 0.f;
    float decalAlphaAccum = 0.f;
    for(uint decalBucketIndex = 0; (decalBucketIndex < NUM_DECAL_BUCKETS) && (decalAlphaAccum < 1.f); ++decalBucketIndex) {
        uint bucket = tiledObjectsIndexList[decalReadOffset + decalBucketIndex];
        [loop]
        while(bucket) {
            const uint indexOfLowestSetBit = firstbitlow(bucket);
            bucket ^= 1 << indexOfLowestSetBit;
            // support max to NUM_DECAL_BUCKETS * 32 decals
            uint decalIndex = decalBucketIndex * 32 + indexOfLowestSetBit;
            // Reverse of operation in culling shader to render the decals front to back. so get right result when early quit due to AlphaAccum
            decalIndex = MAX_NUM_TOTAL_DECALS - decalIndex - 1;
            PbrDecalCB decal = decals[decalIndex];
            float3 offset = surface.P - decal.position;
            // world to decal frame
            float3 local = float3(
                dot(decal.right, offset) / (dot(decal.right, decal.right)),
                dot(decal.up, offset) / (dot(decal.up, decal.up)),
                dot(decal.forward, offset) / (dot(decal.forward, decal.forward))
            );
            float decalStrength = saturate(dot(-surface.N, normalize(decal.forward)));
            [branch]
            if(all(local >= -1.f && local <= 1.f) && decalStrength > 0.f) {
                float2 uv = local.xy * 0.5f + 0.5f;
                float4 viewport = decal.getViewport();
                uv = viewport.xy + uv * viewport.zw;

                const float4 decalAlbedo = decalTextureAtlas.SampleLevel(wrapSampler, uv, 0) * decal.getAlbedo();
                const float decalRoughness = decal.getRoughnessOverride();
                const float decalMetallic = decal.getMetallicOverride();
                const float alpha = decalAlbedo.a * decalStrength;
                const float oneMinusAlphaAccum = 1.f - decalAlphaAccum;
                //blend
                decalAlbedoAccum += oneMinusAlphaAccum * (alpha * decalAlbedo.rgb);
                decalRoughnessAccum += oneMinusAlphaAccum * (alpha * decalRoughness);
                decalMetallicAccum += oneMinusAlphaAccum * (alpha * decalMetallic);
                decalAlphaAccum = alpha + (1.f - alpha) * decalAlphaAccum;
                [branch]
                if(decalAlphaAccum >= 1.f) {
                    decalAlphaAccum = 1.f;
                    break;
                }
            }
        }
    }
    surface.albedo.rgb = lerp(surface.albedo.rgb, decalAlbedoAccum, decalAlbedoAccum);
    surface.roughness = lerp(surface.roughness, decalRoughnessAccum, decalAlphaAccum);
    surface.metallic = lerp(surface.metallic, decalMetallicAccum, decalAlphaAccum);
    surface.inferRemainingProperties();

    uint i = 0;
    for(; i < pointLightCount; ++i) {
        PointLightCB pointLight = pointLights[tiledObjectsIndexList[lightReadIndex++]];
        LightInfo light;
        light.initializeFromPointLight(surface, pointLight);
        float visibility = 1.f;
        [branch]
        if(pointLight.shadowInfoIndex != -1) {
            PointShadowInfo shadowInfo = pointShadowInfos[pointLight.shadowInfoIndex];
            visibility = samplePointLightShadowMapPCF(surface.P, pointLight.position, shadowMap, shadowInfo.viewport0,
                                                      shadowInfo.viewport1, shadowSampler, lighting.shadowMapTexelSize,
                                                      pointLight.radius);
        }
        [branch]
        if(visibility > 0.f) {
            totalLighting.add(calculateDirectLighting(surface, light), visibility);
        }
    }
}