#pragma once
#include "LightSource.hlsli"
#include "RenderCommand.h"
#include "RenderCommandBuffer.h"
#include "ShadowMapCache.h"

struct OpaqueRenderPass
{
public :
    SortKeyVector<float, StaticDepthOnlyRenderCommand> staticDepthPrepass;
    SortKeyVector<float, DynamicDepthOnlyRenderCommand> dynamicDepthPrepass;
    SortKeyVector<float, AnimatedDepthOnlyRenderCommand> animatedDepthPrePass;

    SortKeyVector<float, StaticDepthOnlyRenderCommand> staticDoubleSideDepthPrePass;
    SortKeyVector<float, DynamicDepthOnlyRenderCommand> dynamicDoubleSideDepthPrepass;
    SortKeyVector<float, AnimatedDepthOnlyRenderCommand> animatedDoubleSideDepthPrePass;
    // sort by index
    RenderCommandBuffer<uint64> pass;
    

    void Sort()
    {
        staticDepthPrepass.Sort();
        dynamicDepthPrepass.Sort();
        animatedDepthPrePass.Sort();

        staticDoubleSideDepthPrePass.Sort();
        dynamicDoubleSideDepthPrepass.Sort();
        animatedDoubleSideDepthPrePass.Sort();

        pass.Sort();
    }
    void Reset()
    {
        staticDepthPrepass.Clear();
        dynamicDepthPrepass.Clear();
        animatedDepthPrePass.Clear();

        staticDoubleSideDepthPrePass.Clear();
        dynamicDoubleSideDepthPrepass.Clear();
        animatedDoubleSideDepthPrePass.Clear();

        pass.Clear();
    }
};

struct TransParentRenderPass
{
    // sort by depth
    RenderCommandBuffer<float> pass;

    void Sort()
    {
        pass.Sort();
    }
    void Reset()
    {
        pass.Clear();
    }
};

struct LdrRenderPass
{
    RenderCommandBuffer<float> ldrPass;
    RenderCommandBuffer<float> overlays;
    std::vector<OutlineRenderCommand> outlines;

    void Sort()
    {
        ldrPass.Sort();
        overlays.Sort();
    }
    void Reset()
    {
        ldrPass.Clear();
        overlays.Clear();
        outlines.clear();
    }
};

struct ShadowRenderPassBase
{
    std::vector<ShadowRenderCommand> staticDrawCalls;
    std::vector<ShadowRenderCommand> dynamicDrawCalls;
    std::vector<ShadowRenderCommand> doubleSidedStaticDrawCalls;
    std::vector<ShadowRenderCommand> doubleSidedDynamicDrawCalls;

    void Reset()
    {
        staticDrawCalls.clear();
        dynamicDrawCalls.clear();
        doubleSidedStaticDrawCalls.clear();
        doubleSidedDynamicDrawCalls.clear();
    }
};

struct SunCascadeRenderPass : ShadowRenderPassBase
{
    mat4 viewProj;
    ShadowMapViewport viewport;
};

struct SunShadowRenderPass
{
    SunCascadeRenderPass cascades[MAX_NUM_SUN_SHADOW_CASCADES];
    uint32 numCascades;
    bool shouldCopyFromCache;

    void Reset(){
        for(uint32 i(0); i < MAX_NUM_SUN_SHADOW_CASCADES; ++i) {
            cascades[i].Reset();
        }
        shouldCopyFromCache = false;
    }
};

struct SpotShadowRenderPass : ShadowRenderPassBase
{
    mat4 viewProj;
    ShadowMapViewport viewport;
    bool shouldCopyFromCache;

    void Reset()
    {
        ShadowRenderPassBase::Reset();
        shouldCopyFromCache = false;
    }
};

struct PointShadowRenderPass : ShadowRenderPassBase
{
    //TODO: split for frustum culling;
    ShadowMapViewport viewport0;
    ShadowMapViewport viewport1;
    vec3 lightPosition;
    float maxDistance;
    bool shouldCopyFromCache0;
    bool shouldCopyFromCache1;

    void reset()
    {
        ShadowRenderPassBase::Reset();
        shouldCopyFromCache0 = false;
        shouldCopyFromCache1 = false;
    }
    
};