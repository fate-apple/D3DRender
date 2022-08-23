#pragma once
#include "core/reflect.h"
#include "dx/DxCommandList.h"

struct HBAO_Settings
{
    float radius = 0.5f;
    uint32 numRays;
    uint32 maxNumStepsPerRay = 10;
    float strength = 1.f;
};
//template<> struct TypeDescriptor<HbaoSettings> : MemberList<>{};
REFLECT_STRUCT(HBAO_Settings, (radius), (numRays), (maxNumStepsPerRay), (strength));

static DxPipeline hbaoPipeline;

void ambientOcclusion(DxCommandList* cl,
    ref<DxTexture> linearDepth,					// NON_PIXEL_SHADER_RESOURCE
    ref<DxTexture> screenVelocitiesTexture,		// NON_PIXEL_SHADER_RESOURCE
    ref<DxTexture> aoCalculationTexture,			// UNORDERED_ACCESS
    ref<DxTexture> aoBlurTempTexture,				// UNORDERED_ACCESS
    ref<DxTexture> history,						// NON_PIXEL_SHADER_RESOURCE
    ref<DxTexture> output,							// UNORDERED_ACCESS
    HBAO_Settings settings,
    DxDynamicConstantBuffer cameraCBV);

inline void LoadAoShaders()
{
    hbaoPipeline = CreateReloadablePipeline("HBAO_cs");
}