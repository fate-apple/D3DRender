#pragma once
#include "core/reflect.h"
#include "dx/DxCommandList.h"
#include "dx/DxPipeline.h"

struct TAA_Settings
{
    float cameraJitterStrength = 1.f;
};
REFLECT_STRUCT(TAA_Settings,
    (cameraJitterStrength, "Camera jitter strength")
);

void temporalAntiAliasing(DxCommandList* cl,
    ref<DxTexture> hdrInput,						// NON_PIXEL_SHADER_RESOURCE
    ref<DxTexture> screenVelocitiesTexture,		// NON_PIXEL_SHADER_RESOURCE
    ref<DxTexture> depthStencilBuffer,				// NON_PIXEL_SHADER_RESOURCE
    ref<DxTexture> history,						// NON_PIXEL_SHADER_RESOURCE. After call UNORDERED_ACCESS.
    ref<DxTexture> output,							// UNORDERED_ACCESS. After call NON_PIXEL_SHADER_RESOURCE.
    vec4 jitteredCameraProjectionParams);
static DxPipeline taaPipeline;

inline void LoadAaShaders()
{
    taaPipeline = CreateReloadablePipeline("taa_cs");;
}
