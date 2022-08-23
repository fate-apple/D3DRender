#pragma once
#include "core/math.h"              //used by Camera.hlsli.
#include "core/Camera.h"
#include "core/input.h"
#include "RenderAlgorithms.h"
#include "Camera.hlsli"
#include "LightSource.hlsli"
#include "pbr.h"
#include "RenderUtils.h"
#include "../../shaders/renderAlgorithms/lightProbe/LightProbe.h"
#include "../../shaders/renderAlgorithms/raytracing/RaytracingTLAS.h"

#define MAX_NUM_SUN_LIGHT_SHADOW_PASSES 16
#define MAX_NUM_SPOT_LIGHT_SHADOW_PASSES 16
#define MAX_NUM_POINT_LIGHT_SHADOW_PASSES 16

enum RendererMode
{
    RendererModeRasterized,
    RendererModePathTraced,

    RendererModeCount
};
enum AspectRatioMode
{
    AspectRatioFree,
    AspectRatioFix_16_9,

    AspectRatioModeCount
};
//TODO: sjw. use render graph to decouple feature;
struct RendererSettings
{
    ToneMapSettings toneMapSettings;
    float environmentIntensity = 1.f;
    float skyIntensity = 1.f;

    bool enableAO = true;
    HBAO_Settings aoSettings;
    bool enableSSS = true;
    SSS_Settings sssSettings;
    bool enableSSR = true;
    SSR_Settings ssrSettings;
    bool enableTAA = true;
    TAA_Settings taaSettings;
    bool enableBloom = true;
    Bloom_Settings bloomSettings;
    bool enableSharpen = true;
    Sharpen_Settings sharpenSettings;
};

struct RendererSpec
{
    bool allowObjectPicking = true;
    bool allowAO = true;
    bool allowSSS = true;
    bool allowSSR = true;
    bool allowTAA = true;
    bool allowBloom = true;
};
struct MainRenderer
{
    MainRenderer() = default;

    // simple viewport recalculate
    RendererMode mode = RendererModeRasterized;
    AspectRatioMode aspectRatioMode = AspectRatioFree;
    RendererSettings rendererSettings;

    const RendererSpec spec;
    uint32 renderWidth;
    uint32 renderHeight;
    ref<DxTexture> frameResult;

    void Initialize(color_depth colorDepth, uint32 windowWidth, uint32 windowHeight, RendererSpec spec);
    static void BeginFrameCommon();
    void BeginFrame(uint32 windowWidth, uint32 windowHeight);
    static void RenderFrameCommon();
    void RenderFrame(const UserInput& input);

    //called by application
    void SetCamera(const RenderCamera& camera);
    void SubmitRenderPass(OpaqueRenderPass* renderPass) {assert(!opaqueRenderPass); opaqueRenderPass = renderPass;}
    void SubmitRenderPass(TransParentRenderPass* renderPass) { assert(!transparentRenderPass); transparentRenderPass = renderPass; }
    void SubmitRenderPass(LdrRenderPass* renderPass) { assert(!ldrRenderPass); ldrRenderPass = renderPass; }
    static void SubmitShadowRenderPass(SunShadowRenderPass* renderPass) { assert(numSunLightShadowRenderPasses < MAX_NUM_SUN_LIGHT_SHADOW_PASSES); sunShadowRenderPasses[numSunLightShadowRenderPasses++] = renderPass; }
    static void SubmitShadowRenderPass(SpotShadowRenderPass* renderPass) { assert(numSpotLightShadowRenderPasses < MAX_NUM_SPOT_LIGHT_SHADOW_PASSES);	spotLightShadowRenderPasses[numSpotLightShadowRenderPasses++] = renderPass; }
    static void SubmitShadowRenderPass(PointShadowRenderPass* renderPass) { assert(numPointLightShadowRenderPasses < MAX_NUM_POINT_LIGHT_SHADOW_PASSES); pointLightShadowRenderPasses[numPointLightShadowRenderPasses++] = renderPass; }
    static void RaytraceLightProbes(const LightProbeGrid& grid) { lightProbeGrid = &grid; }
    
private:
    static const SunShadowRenderPass* sunShadowRenderPasses[MAX_NUM_SUN_LIGHT_SHADOW_PASSES];
    static const SpotShadowRenderPass* spotLightShadowRenderPasses[MAX_NUM_SPOT_LIGHT_SHADOW_PASSES];
    static const PointShadowRenderPass* pointLightShadowRenderPasses[MAX_NUM_POINT_LIGHT_SHADOW_PASSES];
    static uint32 numSunLightShadowRenderPasses;
    static uint32 numSpotLightShadowRenderPasses;
    static uint32 numPointLightShadowRenderPasses;

    static ref<PbrEnvironment> environment;
    static DirectionalLightCB sun;
    static DxDynamicConstantBuffer sunCBV;
    static ref<DxBuffer> pointLights;
    static ref<DxBuffer> spotLights;
    static ref<DxBuffer> pointLightShadowInfoBuffer;
    static ref<DxBuffer> spotLightShadowInfoBuffer;
    static ref<DxBuffer> decals;
    static uint32 numPointLights;
    static uint32 numSpotLights;
    static uint32 numDecals;
    static const LightProbeGrid* lightProbeGrid;
    static RaytracingTLAS* tlas;
    PathTracer pathTracer;
    
    AspectRatioMode oldAspectRatioMode = AspectRatioFree;
    
    uint32 windowWidth;
    uint32 windowHeight;
    int32 windowXOffset = 0;
    int32 windowYOffset = 0;

    // Create when initialize
    ref<DxTexture> hdrColorTexture;
    ref<DxTexture> prevFrameHDRColorTexture; 
    ref<DxTexture> prevFrameHDRColorTempTexture;
    
    ref<DxTexture> worldNormalRoughnessTexture;
    ref<DxTexture> screenVelocityTexture;
    ref<DxTexture> linearDepthBuffer;
    ref<DxTexture> opaqueDepthBuffer;
    ref<DxTexture> depthStencilBuffer;
    ref<DxTexture> objectIdsTexture;

    ref<DxTexture> aoCalculationTexture;    //8bit
    ref<DxTexture> aoBlurTempTexture;
    ref<DxTexture> aoTextures[2];
    bool isAoOnPrevFrame = false;
    uint32 aoHistoryIndex = 0;

    ref<DxTexture> sssCalculationTexture;   //8bit
    ref<DxTexture> sssBlurTempTexture;
    ref<DxTexture> sssTextures[2];
    bool isSssOnPrevFrame = false;
    uint32 sssHistoryIndex = 0;

    ref<DxTexture> ssrRaycastTexture;
    ref<DxTexture> ssrResolveTexture;
    ref<DxTexture> ssrTemporalTextures[2]; // These get flip-flopped from frame to frame.
    uint32 ssrHistoryIndex = 0;
    

    ref<DxTexture> hdrPostProcessingTexture;
    ref<DxTexture> ldrPostProcessingTexture;
    ref<DxTexture> taaTextures[2]; 
    uint32 taaHistoryIndex = 0;

    ref<DxTexture> bloomTexture;
    ref<DxTexture> bloomTempTexture;
    ref<DxBuffer> hoveredObjectIDReadbackBuffer;

    LightCulling _lightCulling;
    CameraCB jitteredCamera;
    CameraCB unJitteredCamera;

    const OpaqueRenderPass* opaqueRenderPass;
    const TransParentRenderPass* transparentRenderPass;
    const LdrRenderPass* ldrRenderPass;
    void RecalculateViewport(bool resizeTextures);
};
