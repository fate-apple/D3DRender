#pragma once
#include "PbrRaytracer.h"
#include "RaytracingTLAS.h"
#include "dx/DxCommandList.h"
#include "rendering/Material.h"


struct PathTracer : PbrRaytracer
{

    const uint32 maxRecursionDepth = 4;
    const uint32 maxPayloadSize = 5 * sizeof(float);     // Radiance-payload is 1 x float3, 2 x uint.

    uint32 numAveragedFrames = 0;
    uint32 recursiveDepth = maxRecursionDepth -1;       //. 0 means, that no primary ray is shot. 1 means that no bounce is computed
    uint32 startRussianRoulette = recursiveDepth;
    bool multipleImportanceSampling = true;

    bool useThinLensCamera = false;
    float fNumber = 32.f;
    float focalLength = 1.f;

    bool useRealMaterial = false;
    bool enableDirectLighting = false;
    float lightIntensityScale = 1.f;
    float pointLightRadius = 0.1f;
    
    void Initialize();
    void Render( DxCommandList* cl, const RaytracingTLAS& tlas, const ref<DxTexture>& output, const CommonMaterialInfo& materialInfo);
private:

    // Only descriptors in here!
    struct InputResources
    {
        DxCpuDescriptorHandle tlas;
        DxCpuDescriptorHandle sky;
    };

    struct OnputResources
    {
        DxCpuDescriptorHandle output;
    };
    
};
