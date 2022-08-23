#pragma once

#include "core/Memory.h"
#include "dx/DxBuffer.h"
#include "editor/Editor.h"
#include "renderAlgorithms/lightProbe/LightProbe.h"
#include "renderAlgorithms/raytracing/RaytracingTLAS.h"
#include "rendering/MainRenderer.h"
#include "scene/Scene.h"
#include "core/input.h"

struct Application
{
    void Initialize(MainRenderer* renderer);
    void LoadCustomShaders();
    void HandleFileDrop(const fs::path& filename);
    void update(const UserInput& input, float dt);

private:

    ref<DxBuffer> pointLightBuffer[NUM_BUFFERED_FRAMES];
    ref<DxBuffer> spotLightBuffer[NUM_BUFFERED_FRAMES];
    ref<DxBuffer> decalBuffer[NUM_BUFFERED_FRAMES];
    ref<DxBuffer> spotLightShadowInfoBuffer[NUM_BUFFERED_FRAMES];
    ref<DxBuffer> pointLightShadowInfoBuffer[NUM_BUFFERED_FRAMES];

    MainRenderer* renderer;
    EditorScene scene;
    scene_editor editor;
    MemoryArena stackArena;
    light_probe_grid lightProbeGrid;
    RaytracingTLAS raytracingTLAS;
    
    OpaqueRenderPass _opaqueRenderPass;
    TransParentRenderPass _transParentPass;
    LdrRenderPass _ldrRenderPass;
    SunShadowRenderPass sunShadowRenderPass;
    SpotShadowRenderPass spotShadowRenderPasses[MAX_NUM_SPOT_LIGHT_SHADOW_PASSES];
    PointShadowRenderPass pointShadowRenderPasses[MAX_NUM_POINT_LIGHT_SHADOW_PASSES];
    
    void SubmitRendererParams(uint32 numSpotLightShadowPasses, uint32 numPointLightShadowPasses);
    
};
