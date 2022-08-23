#pragma once
#include "renderAlgorithms/raytracing/PbrRaytracer.h"
#include "core/Math.h"
#include "ssr_rs.hlsli"
#include "dx/DxCommandList.h"
#include "renderAlgorithms/IBL/ImageBasedLighting.h"
static DxPipeline ssrRaycastPipeline;
static DxPipeline ssrResolvePipeline;
static DxPipeline ssrTemporalPipeline;
static DxPipeline ssrMedianBlurPipeline;

class SpecularReflectionsRaytracer : PbrRaytracer
{
    uint32 numBounces = 1;
    float fadeoutDistance = 80.f;
    float maxDistance = 100.f;

private:
    struct InputResource
    {
        DxCpuDescriptorHandle tlas;
        DxCpuDescriptorHandle environment;
        DxCpuDescriptorHandle sky;
        DxCpuDescriptorHandle irradiance;
        DxCpuDescriptorHandle depthBuffer;
        DxCpuDescriptorHandle screenSpaceNormals;
        DxCpuDescriptorHandle brdf;
    };

    struct OnputResource
    {
        DxCpuDescriptorHandle output;
    };
    const uint32 maxRecursionDepth = 4;
};

void ScreenSpaceReflections(DxCommandList* cl, ref<DxTexture> raycastTexture, ref<DxTexture> depthStencilBuffer,
                            ref<DxTexture> linearDepthBuffer, ref<DxTexture> worldNormalsRoughnessTexture,
                            ref<DxTexture> screenVelocitiesTexture, ref<DxTexture> prevFrameHdr, ref<DxTexture> resolveTexture,
                            ref<DxTexture> ssrTemporalHistory,
                            ref<DxTexture> ssrTemporalOutput,
                            SSR_Settings settings,
                            DxDynamicConstantBuffer cameraCBV);

inline void LoadSsrShaders()
{
    ssrRaycastPipeline = CreateReloadablePipeline("SSR_Raycast_cs");
    ssrResolvePipeline = CreateReloadablePipeline("SSR_Resolve_cs");
    ssrTemporalPipeline = CreateReloadablePipeline("SSR_Temporal_cs");
    ssrMedianBlurPipeline = CreateReloadablePipeline("SSR_MedianBlur_cs");
}