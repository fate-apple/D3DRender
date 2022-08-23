#include "Brdf.hlsli"
#include "Camera.hlsli"
#include "ssr_rs.hlsli"
#include "cs.hlsli"
#include "Math.hlsli"
#include "normal.hlsli"

ConstantBuffer<SSR_ResolveCB> cb : register(b0);
ConstantBuffer<CameraCB> camera : register(b1);

Texture2D<float> depthBuffer : register(t0);
Texture2D<float3> worldNormalsRoughness : register(t1);
Texture2D<float4> reflection : register(t2);
Texture2D<float4> hdrColor : register(t3);
Texture2D<float2> motion : register(t4); //screen Velocities

RWTexture2D<float4> output : register(u0);

SamplerState linearSampler : register(s0);
SamplerState pointSampler : register(s1);


// Based on https://github.com/simeonradivoev/ComputeStochasticReflections

/*
Copyright (c) 2018 Simeon Radivoev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define SSR_RESOLVE_RAD  4
#define SSR_RESOLVE_RAD2 (SSR_RESOLVE_RAD * 2)

groupshared uint groupR[(SSR_BLOCK_SIZE + SSR_RESOLVE_RAD2) * (SSR_BLOCK_SIZE + SSR_RESOLVE_RAD2)];
groupshared uint groupG[(SSR_BLOCK_SIZE + SSR_RESOLVE_RAD2) * (SSR_BLOCK_SIZE + SSR_RESOLVE_RAD2)];
groupshared uint groupB[(SSR_BLOCK_SIZE + SSR_RESOLVE_RAD2) * (SSR_BLOCK_SIZE + SSR_RESOLVE_RAD2)];
groupshared uint groupA[(SSR_BLOCK_SIZE + SSR_RESOLVE_RAD2) * (SSR_BLOCK_SIZE + SSR_RESOLVE_RAD2)];

static void packResolveData(uint index, float4 raycast, float4 color)
{
    groupR[index] = f32tof16(raycast.r) | (f32tof16(color.r) << 16);
    groupG[index] = f32tof16(raycast.g) | (f32tof16(color.g) << 16);
    groupB[index] = f32tof16(raycast.b) | (f32tof16(color.b) << 16);
    groupA[index] = f32tof16(raycast.a) | (f32tof16(color.a) << 16);
}
static void getResolveData(uint index, out float4 raycast, out float4 color)
{
    uint rr = groupR[index];
    uint gg = groupG[index];
    uint bb = groupB[index];
    uint aa = groupA[index];
    raycast = float4(f16tof32(rr), f16tof32(gg), f16tof32(bb), f16tof32(aa));
    color = float4(f16tof32(rr >> 16), f16tof32(gg >> 16), f16tof32(bb >> 16), f16tof32(aa >> 16));
}


static const int2 resolveOffset[] =
{
    int2(0, 0),
    int2(0, 1),
    int2(1, -1),
    int2(-1, -1),
};




[numthreads(SSR_BLOCK_SIZE + SSR_RESOLVE_RAD2, SSR_BLOCK_SIZE + SSR_RESOLVE_RAD2, 1)]
[RootSignature(SSR_RESOLVE_RS)]
void main(CsInput In)
{
    const uint2 uvInt = (In.groupID.xy * SSR_BLOCK_SIZE) + In.groupThreadID.xy - SSR_RESOLVE_RAD;
    const float2 uv = (uvInt.xy + 0.5f) * cb.invDimensions;

    const float2 uvM = motion.SampleLevel(linearSampler, uv, 0);
    const float3 normalAndRoughness = worldNormalsRoughness.SampleLevel(linearSampler, uv + uvM, 0);
    const float3 normal = unpackNormal(normalAndRoughness.xy);
    const float3 N = normalize(mul(camera.view, float4(normal, 0.f)).xyz);
    const float roughness = clamp(normalAndRoughness.z, 0.03f, 0.97f);

    const float depth = depthBuffer.SampleLevel(pointSampler, uv, 0);
    const float3 viewPos = RevertViewSpacePosition(camera.invProj, uv, depth);      //shading point position in view space
    const float3 V = normalize(-viewPos);

    const float NdotV = saturate(dot(N, V));
    const float sqrtRoughness = sqrt(roughness);
    float coneTangent = lerp(0.f, roughness * (1.f - SSR_GGX_IMPORTANCE_SAMPLE_BIAS), NdotV * sqrtRoughness);
    coneTangent *= lerp(saturate(NdotV * 2.f), 1.f, sqrtRoughness);

    // xy :hitPixel, z: hitDepth in depth buffer( < 0 if no hit), w: pdf of importance sampling GGX
    float4 raycastResult = reflection.SampleLevel(linearSampler, uv, 0);
    float hit = raycastResult.z > 0.f;
    raycastResult.z = abs(raycastResult.z); // Remove sign.
    float sourceMip = clamp(log2(coneTangent * max(cb.dimensions.x, cb.dimensions.y)), 0.f, 4.f);

    const float2 m = motion.SampleLevel(linearSampler, raycastResult.xy, 0);
    float4 sceneColor = hdrColor.SampleLevel(linearSampler, raycastResult.xy + m, sourceMip);
    // This makes no sense at all physically. For some reason it helps against light "feedback" where the scene gets very bright.
    // how to comprehend game202 lecture9 94:00 : not introduce square distance falloff?
    sceneColor.rgb *= pow(0.2f, sourceMip);
    sceneColor.w = hit;

    packResolveData(In.groupIndex, raycastResult, sceneColor);

    GroupMemoryBarrierWithGroupSync();

    if(In.groupThreadID.x < SSR_RESOLVE_RAD | In.groupThreadID.y < SSR_RESOLVE_RAD | In.groupThreadID.x >= SSR_BLOCK_SIZE +
        SSR_RESOLVE_RAD || In.groupThreadID.y >= SSR_BLOCK_SIZE + SSR_RESOLVE_RAD) {
        return;
    }
    if(depth == 1.f) {
        output[uvInt.xy] = float4(0.f, 0.f, 0.f, 0.f);
        return;
    }
    float4 result = float4(0.f, 0.f, 0.f, 0.f);
    float totalWeight = 0.f;
    static const float3 luminanceWeights = float3(0.2126f, 0.7152f, 0.0722f);
    const float borderAttenuationDistance = 0.25f;
    [unroll]
    for(uint i = 0; i < 4; ++i) {
        int2 offset = resolveOffset[i] * 2;
        int2 neighborGroupThreadID = In.groupThreadID.xy + offset;
        uint neighborIndex = flatten2D(neighborGroupThreadID, SSR_BLOCK_SIZE + SSR_RESOLVE_RAD2);

        float4 neighborRaycastResult;
        float4 neighborSceneColor;
        getResolveData(neighborIndex, neighborRaycastResult, neighborSceneColor);

        float2 neighborUV = neighborRaycastResult.xy;
        float neighborDepth = neighborRaycastResult.z;
        float neighborPDF = neighborRaycastResult.w;

        float neighborHit = neighborSceneColor.a;

        const float3 neighborViewPos = camera.RevertViewSpacePosition(neighborUV, neighborDepth);
        
        // BRDF weight.
        float3 L = normalize(neighborViewPos - viewPos);
        float3 H = normalize(L + V);

        float NdotH = saturate(dot(N, H));
        float NdotL = saturate(dot(N, L));

        float G = geometrySmith(NdotL, NdotV, roughness);
        float D = distributionGGX(NdotH, roughness);
        float specularLight = G * D * M_PI / 4.f;

        float weight = specularLight / max(neighborPDF, 0.00001f);


        float borderDist = min(1.f - max(neighborUV.x, neighborUV.y), min(neighborUV.x, neighborUV.y));
        float borderAttenuation = saturate(borderDist / borderAttenuationDistance);

        neighborSceneColor.a = borderAttenuation * neighborHit;
        neighborSceneColor.rgb /= 1.f + dot(neighborSceneColor.rgb, luminanceWeights);

        result += neighborSceneColor * weight;
        totalWeight += weight;
    }
    result /= totalWeight;
    result.rgb /= 1.f - dot(result.rgb, luminanceWeights);
    
    output[uvInt.xy] = max(1e-5f, result);                  // w is hit

    //output[uvInt.xy] = float4(raycastResult.xy, 0.f, 1.f);
}