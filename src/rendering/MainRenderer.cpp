#include "pch.h"
#include "MainRenderer.h"

#include "Material.h"
#include "pbr.h"
#include "RenderResources.h"
#include "RenderUtils.h"
#include "core/Thread.h"
#include "dx/DxBarrierBatcher.h"
#include "dx/DxContext.h"
#include "dx/DxProfiling.h"


//ref<PbrEnvironment> MainRenderer::environment;
#define SSR_RAYCAST_WIDTH (renderWidth / 2)
#define SSR_RAYCAST_HEIGHT (renderHeight / 2)

#define SSR_RESOLVE_WIDTH (renderWidth / 2)
#define SSR_RESOLVE_HEIGHT (renderHeight / 2)

void MainRenderer::Initialize(color_depth colorDepth, uint32 windowWidth, uint32 windowHeight, RendererSpec spec)
{
    this->windowWidth = windowWidth;
    this->windowHeight = windowHeight;
    const_cast<RendererSpec&>(this->spec) = spec;

    rendererSettings.enableSharpen = spec.allowTAA;
    RecalculateViewport(false);
    hdrColorTexture = CreateTexture(nullptr, renderWidth, renderHeight, hdrFormat, false, true, true, D3D12_RESOURCE_STATE_RENDER_TARGET);
    SET_NAME(hdrColorTexture->resource, "HDR Color Texture");
    if (spec.allowSSR)
    {
        D3D12_RESOURCE_DESC prevFrameHDRColorDesc = CD3DX12_RESOURCE_DESC::Tex2D(hdrFormat, renderWidth / 2, renderHeight / 2, 1,
            8, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        prevFrameHDRColorTexture = CreateTexture(prevFrameHDRColorDesc, 0, 0, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);
        prevFrameHDRColorTempTexture = CreateTexture(prevFrameHDRColorDesc, 0, 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

        SET_NAME(prevFrameHDRColorTexture->resource, "Prev frame HDR Color");
        SET_NAME(prevFrameHDRColorTempTexture->resource, "Prev frame HDR Color Temp");
    }
    if (spec.allowSSR || spec.allowTAA || spec.allowAO || spec.allowObjectPicking)
    {
        screenVelocityTexture = CreateTexture(0, renderWidth, renderHeight, screenVelocitiesFormat, false, true, false, D3D12_RESOURCE_STATE_RENDER_TARGET);
        SET_NAME(screenVelocityTexture->resource, "Screen velocities");
    }
    worldNormalRoughnessTexture = CreateTexture(0, renderWidth, renderHeight, worldNormalsRoughnessFormat, false, true, false, D3D12_RESOURCE_STATE_GENERIC_READ);
    SET_NAME(worldNormalRoughnessTexture->resource, "World normals / roughness");
    depthStencilBuffer = CreateDepthTexture(renderWidth, renderHeight, depthStencilFormat);
    opaqueDepthBuffer = CreateDepthTexture(renderWidth, renderHeight, depthStencilFormat, 1, D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_RESOURCE_DESC linearDepthDesc = CD3DX12_RESOURCE_DESC::Tex2D(linearDepthFormat, renderWidth, renderHeight, 1,
        6, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    linearDepthBuffer = CreateTexture(linearDepthDesc, 0, 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
    SET_NAME(depthStencilBuffer->resource, "Depth buffer");
    SET_NAME(opaqueDepthBuffer->resource, "Opaque depth buffer");
    SET_NAME(linearDepthBuffer->resource, "Linear depth buffer");

    if (spec.allowAO)
    {
        aoCalculationTexture = CreateTexture(nullptr, renderWidth / 2, renderHeight / 2, aoFormat, false, false, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        aoBlurTempTexture = CreateTexture(nullptr, renderWidth, renderHeight, aoFormat, false, false, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        aoTextures[aoHistoryIndex] = CreateTexture(nullptr, renderWidth, renderHeight, aoFormat, false, false, true, D3D12_RESOURCE_STATE_GENERIC_READ);
        aoTextures[1 - aoHistoryIndex] = CreateTexture(nullptr, renderWidth, renderHeight, aoFormat, false, false, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        SET_NAME(aoCalculationTexture->resource, "AO Calculation");
        SET_NAME(aoBlurTempTexture->resource, "AO Temp");
        SET_NAME(aoTextures[0]->resource, "AO 0");
        SET_NAME(aoTextures[1]->resource, "AO 1");
    }
    if (spec.allowSSS)
    {
        sssCalculationTexture = CreateTexture(0, renderWidth / 2, renderHeight / 2, sssFormat, false, false, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        sssBlurTempTexture = CreateTexture(0, renderWidth, renderHeight, sssFormat, false, false, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        sssTextures[sssHistoryIndex] = CreateTexture(0, renderWidth, renderHeight, sssFormat, false, false, true, D3D12_RESOURCE_STATE_GENERIC_READ);
        sssTextures[1 - sssHistoryIndex] = CreateTexture(0, renderWidth, renderHeight, sssFormat, false, false, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        SET_NAME(sssCalculationTexture->resource, "SSS Calculation");
        SET_NAME(sssBlurTempTexture->resource, "SSS Temp");
        SET_NAME(sssTextures[0]->resource, "SSS 0");
        SET_NAME(sssTextures[1]->resource, "SSS 1");
    }
    if (spec.allowSSR)
    {
        ssrRaycastTexture = CreateTexture(0, SSR_RAYCAST_WIDTH, SSR_RAYCAST_HEIGHT, hdrFormat, false, false, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        ssrResolveTexture = CreateTexture(0, SSR_RESOLVE_WIDTH, SSR_RESOLVE_HEIGHT, hdrFormat, false, false, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        ssrTemporalTextures[ssrHistoryIndex] = CreateTexture(0, SSR_RESOLVE_WIDTH, SSR_RESOLVE_HEIGHT, hdrFormat, false, false, true, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        ssrTemporalTextures[1 - ssrHistoryIndex] = CreateTexture(0, SSR_RESOLVE_WIDTH, SSR_RESOLVE_HEIGHT, hdrFormat, false, false, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        SET_NAME(ssrRaycastTexture->resource, "SSR Raycast");
        SET_NAME(ssrResolveTexture->resource, "SSR Resolve");
        SET_NAME(ssrTemporalTextures[0]->resource, "SSR Temporal 0");
        SET_NAME(ssrTemporalTextures[1]->resource, "SSR Temporal 1");
    }
    if (spec.allowTAA)
    {
        taaTextures[taaHistoryIndex] = CreateTexture(0, renderWidth, renderHeight, hdrFormat, false, true, true, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        taaTextures[1 - taaHistoryIndex] = CreateTexture(0, renderWidth, renderHeight, hdrFormat, false, true, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        SET_NAME(taaTextures[0]->resource, "TAA 0");
        SET_NAME(taaTextures[1]->resource, "TAA 1");
    }

    if (spec.allowBloom)
    {
        D3D12_RESOURCE_DESC bloomDesc = CD3DX12_RESOURCE_DESC::Tex2D(hdrFormat, renderWidth / 4, renderHeight / 4, 1,
            5, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        bloomTexture = CreateTexture(bloomDesc, 0, 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
        bloomTempTexture = CreateTexture(bloomDesc, 0, 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

        SET_NAME(bloomTexture->resource, "Bloom");
        SET_NAME(bloomTempTexture->resource, "Bloom Temp");
    }

    hdrPostProcessingTexture = CreateTexture(0, renderWidth, renderHeight, hdrFormat, false, true, true, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    SET_NAME(hdrPostProcessingTexture->resource, "HDR Post processing");


    ldrPostProcessingTexture = CreateTexture(0, renderWidth, renderHeight, ldrFormat, false, true, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    SET_NAME(ldrPostProcessingTexture->resource, "LDR Post processing");

    if (spec.allowObjectPicking)
    {
        hoveredObjectIDReadbackBuffer = CreateReadBackBuffer(GetFormatSize(objectIDsFormat), NUM_BUFFERED_FRAMES);

        objectIdsTexture = CreateTexture(0, renderWidth, renderHeight, objectIDsFormat, false, true, false, D3D12_RESOURCE_STATE_RENDER_TARGET);
        SET_NAME(objectIdsTexture->resource, "Object IDs");
    }

    if (dxContext.featureSupport.Raytracing())
    {
        pathTracer.initialize();
    }
    frameResult = CreateTexture(0, windowWidth, windowHeight, ColorDepthToFormat(colorDepth), false, true, true);
    SET_NAME(frameResult->resource, "Frame result");
}


void MainRenderer::RecalculateViewport(bool resizeTextures)
{
    if(aspectRatioMode == AspectRatioFree) {
        renderWidth = windowWidth;
        renderHeight = windowHeight;
    } else if(aspectRatioMode == AspectRatioFix_16_9) {
        const float aspectTarget = (16.f / 9.f);
        float aspectCur = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
        if(aspectCur > aspectTarget) {
            renderWidth = static_cast<uint32>(floor(windowHeight * aspectTarget));
            renderHeight = windowHeight;
            windowXOffset = static_cast<int32>((windowWidth - renderWidth) / 2);
            windowYOffset = 0;
        } else {
            renderWidth = windowWidth;
            renderHeight = static_cast<uint32>(windowWidth / aspectTarget);
            windowXOffset = 0;
            windowYOffset = static_cast<int32>((windowHeight - renderHeight) / 2);
        }
    }
    if(resizeTextures) {
        ResizeTexture(hdrColorTexture, renderWidth, renderHeight);
        ResizeTexture(prevFrameHDRColorTexture, renderWidth / 2, renderHeight / 2);
        ResizeTexture(prevFrameHDRColorTempTexture, renderWidth / 2, renderHeight / 2);

        ResizeTexture(worldNormalRoughnessTexture, renderWidth, renderHeight);
        ResizeTexture(screenVelocityTexture, renderWidth, renderHeight);
        ResizeTexture(depthStencilBuffer, renderWidth, renderHeight);
        ResizeTexture(opaqueDepthBuffer, renderWidth, renderHeight);
        ResizeTexture(linearDepthBuffer, renderWidth, renderHeight);
    }
    _lightCulling.AllocateIfNecessary(renderWidth, renderHeight);
}

void MainRenderer::RenderFrameCommon()
{
    sunCBV  = dxContext.UploadDynamicConstantBuffer(sun);
    DxCommandList* cl = dxContext.GetFreeRenderCommandList();

    {
        PROFILE_ALL(cl, "Shadow Maps")
        cl->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cl->TransitionBarrier(RenderResources::shadowMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        ShadowPass(cl, sunShadowRenderPasses, numSunLightShadowRenderPasses,
            spotLightShadowRenderPasses, numSpotLightShadowRenderPasses,
            pointLightShadowRenderPasses, numPointLightShadowRenderPasses);
        cl->TransitionBarrier(RenderResources::shadowMap, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    if (dxContext.featureSupport.Raytracing() && lightProbeGrid && tlas)
    {
        PROFILE_ALL(cl, "Update light probes");

        dxContext.renderQueue.WaitForOtherQueue(dxContext.computeQueue); // Wait for AS-rebuilds. TODO: This is not the way to go here. We should wait for the specific value returned by executeCommandList.

        lightProbeGrid->UpdateProbes(cl, *tlas, environment ? environment->sky : 0, sunCBV);
    }
    dxContext.ExecuteCommandList(cl);
}

void MainRenderer::RenderFrame(const UserInput& input)
{
    bool aspectRatioModeChanged = aspectRatioMode != oldAspectRatioMode;
    oldAspectRatioMode = aspectRatioMode;
    if(aspectRatioModeChanged) {
        RecalculateViewport(true);
    }
    DxDynamicConstantBuffer jitteredCameraCBV = dxContext.UploadDynamicConstantBuffer(jitteredCamera);
    DxDynamicConstantBuffer unJitteredCameraCBV = dxContext.UploadDynamicConstantBuffer(unJitteredCamera);

    rendererSettings.enableAO &= spec.allowAO;
    rendererSettings.enableSSS &= spec.allowSSS;
    rendererSettings.enableSSR &= spec.allowSSR;
    rendererSettings.enableBloom &= spec.allowBloom;
    rendererSettings.enableTAA &= spec.allowTAA;

    CommonMaterialInfo materialInfo;
    if(environment) {
        materialInfo.sky = environment->sky;
        materialInfo.environment = environment->environment;
        materialInfo.irradiance = environment->irradiance;
    } else {
        materialInfo.sky = RenderResources::blackCubeTexture;
        materialInfo.environment = RenderResources::blackCubeTexture;
        materialInfo.irradiance = RenderResources::blackCubeTexture;
    }
    materialInfo.environmentIntensity = rendererSettings.environmentIntensity;
    materialInfo.skyIntensity = rendererSettings.skyIntensity;
    materialInfo.aoTexture = rendererSettings.enableAO
                                 ? aoTextures[NUM_BUFFERED_FRAMES - 1 - aoHistoryIndex]
                                 : RenderResources::whiteTexture;
    materialInfo.sssTexture = rendererSettings.enableSSS
                                  ? sssTextures[NUM_BUFFERED_FRAMES - 1 - sssHistoryIndex]
                                  : RenderResources::whiteTexture;
    materialInfo.ssrTexture = rendererSettings.enableSSR ? ssrResolveTexture : nullptr;
    materialInfo.tiledCullingGrid = _lightCulling.tiledCullingGrid;
    materialInfo.tiledObjectsIndexList = _lightCulling.tiledObjectsIndexBuffer;
    materialInfo.pointLightBuffer = pointLights;
    materialInfo.spotLightBuffer = spotLights;
    materialInfo.decalBuffer = decals;
    materialInfo.shadowMap = RenderResources::shadowMap;
    if(mode == RendererModeRasterized) {
        DxCommandList* cls[3];
        ThreadJobContext jobContext;
        jobContext.AddWork([&]()
        {
            DxCommandList* cl = dxContext.GetFreeRenderCommandList();
            PROFILE_ALL(cl, "render thread 1");

            if(aspectRatioModeChanged) {
                cl->TransitionBarrier(frameResult, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
                cl->ClearRTV(frameResult, 0.f, 0.f, 0.f);
                cl->TransitionBarrier(frameResult, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
            }
            cl->ClearDepthAndStencil(depthStencilBuffer); //depth => 1, stencil => 0 as default;
            WaitForSkinningToFinish();

            /*--------------------------------------------- DEPTH-ONLY PASS ---------------------------------*/
            auto depthOnlyRenderTarget = DxRenderTarget(renderWidth, renderHeight).ColorAttachMent(screenVelocityTexture,
                                                                                      RenderResources::nullScreenVelocitiesRTV)
                                                                                  .ColorAttachMent(
                                                                                      objectIdsTexture,
                                                                                      RenderResources::nullObjectIDsRTV).
                                                                                  depthAttachment(depthStencilBuffer);
            DepthPrePass(cl, depthOnlyRenderTarget, opaqueRenderPass, jitteredCamera.viewProj, jitteredCamera.prevFrameViewProj,
                         jitteredCamera.jitter, jitteredCamera.prevFrameJitter);
            //The resource is used with a shader other than the pixel shader. A subresource must be in this state before being read by any stage
            BarrierBatcher(cl).Transition(depthStencilBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            LightAndDecalCulling(cl, depthStencilBuffer, pointLights, spotLights, decals, _lightCulling, numPointLights, numSpotLights, numDecals, materialInfo.cameraCBV);
            // ----------------------------------------
            // LINEAR DEPTH PYRAMID
            // ----------------------------------------

            LinearDepthPyramid(cl, depthStencilBuffer, linearDepthBuffer, jitteredCamera.projectionParams);


            BarrierBatcher(cl)
                //.uav(linearDepthBuffer)
                .Transition(linearDepthBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
                .Transition(depthStencilBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

            cls[0] = cl;
        });

        jobContext.AddWork([&]()
        {
            DxCommandList* cl = dxContext.GetFreeRenderCommandList();
            PROFILE_ALL(cl, "Render thread 2");

        // ----------------------------------------
        // SCREEN SPACE REFLECTIONS
        // ----------------------------------------

        if (rendererSettings.enableSSR)
        {
            BarrierBatcher(cl)
                .Transition(depthStencilBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            ScreenSpaceReflections(cl, ssrRaycastTexture, depthStencilBuffer, linearDepthBuffer, worldNormalRoughnessTexture,
                screenVelocityTexture, prevFrameHDRColorTexture, ssrResolveTexture, ssrTemporalTextures[ssrHistoryIndex],
                ssrTemporalTextures[1 - ssrHistoryIndex], rendererSettings.ssrSettings, materialInfo.cameraCBV);

            ssrHistoryIndex = 1 - ssrHistoryIndex;

            BarrierBatcher(cl)
                .Transition(depthStencilBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        }
        });
    }
}