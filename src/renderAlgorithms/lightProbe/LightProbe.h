#pragma once
#include "core/math.h"
#include "dx/DxTexture.h"
#include "rendering/pbr.h"
#include "rendering/RenderPass.h"


struct light_probe_grid
{
    vec3 minCorner;
    float cellSize;

    uint32 numNodesX;
    uint32 numNodesY;
    uint32 numNodesZ;
    uint32 totalNumNodes = 0;

    ref<DxTexture> irradiance;
    ref<DxTexture> depth;

    ref<DxTexture> raytracedRadiance;
    ref<DxTexture> raytracedDirectionAndDistance;

    void initialize(vec3 minCorner, vec3 dimensions, float cellSize);
    void visualize(OpaqueRenderPass* renderPass, const ref<PbrEnvironment>& environment);

    void updateProbes(DxCommandList* cl, const raytracing_tlas& lightProbeTlas, const ref<DxTexture>& sky, DxDynamicConstantBuffer sunCBV) const;

    LightProbeGridCB getCB() const { return { minCorner, cellSize, numNodesX, numNodesY, numNodesZ }; }


private:
    bool visualizeProbes = false;
    bool visualizeRays = false;
    bool showTestSphere = false;
    bool autoRotateRays = true;

    bool rotateRays = false;

    float irradianceUIScale = 1.f;
    float depthUIScale = 1.f;
    float raytracedRadianceUIScale = 1.f;

    mutable quat rayRotation = quat::identity;
    mutable RandomNumbterGenerator rng = { 1996 };

    friend struct light_probe_tracer;
};
