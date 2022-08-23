#include "pch.h"
#include "RenderAlgorithms.h"

#include "DepthOnly_rs.hlsli"
#include "PostProcessing_rs.hlsli"
#include "Sky_rs.hlsli"
#include "RenderCommand.h"
#include "RenderCommandBuffer.h"

#include "dx/DxPipeline.h"
#include "dx/DxProfiling.h"
#include "RenderUtils.h"
#include "geometry/mesh_builder.h"

//PSOs
static DxPipeline depthPrePassPipeline;
static DxPipeline animatedDepthPrePassPipeline;
static DxPipeline doubleSideDepthPrePassPipeline;
static DxPipeline animatedDoubleSideDepthPrePassPipeline;

static DxPipeline hierarchicalLinearDepthPipeline;
static DxPipeline shadowBlurXPipeline;
static DxPipeline shadowBlurYPipeline;

static DxPipeline shadowPipeline;
static DxPipeline pointLightShadowPipeline;
static DxPipeline doubleSidedShadowPipeline;
static DxPipeline doubleSidedPointLightShadowPipeline;
static DxPipeline shadowMapCopyPipeline;

static DxPipeline textureSkyPipeline;
static DxPipeline proceduralSkyPipeline;
static DxPipeline preethamSkyPipeline;
static DxPipeline sphericalHarmonicsSkyPipeline;

void LoadCommonShaders()
{
    {
        auto desc = CREATE_GRAPHICS_PSO
                    .RenderTargets(skyPassFormats, arraySize(skyPassFormats), depthStencilFormat)
                    .DepthSettings(true, false)
                    .CullFrontFaces();
        textureSkyPipeline = CreateReloadablePipeline(desc, {{"Sky_vs", "SkyTexture_ps"}}, RSInPS);
        proceduralSkyPipeline = CreateReloadablePipeline(desc, {{"Sky_vs", "SkyProcedural_ps"}}, RSInPS);
        //preethamSkyPipeline = CreateReloadablePipeline(desc, {{"Sky_vs", "SkyPreetham_ps"}}, RSInPS);
        //sphericalHarmonicsSkyPipeline = CreateReloadablePipeline(desc, {{"Sky_vs", "SkySH_ps"}}, RSInPS);
    }
    {
        auto desc = CREATE_GRAPHICS_PSO
                    .RenderTargets(depthOnlyFormats, arraySize(depthOnlyFormats), depthStencilFormat)
                    .InputLayout(inputLayout_position);
        depthPrePassPipeline = CreateReloadablePipeline(desc, {{"DepthOnly_vs", "DepthOnly_ps"}}, RsInVS);
        animatedDepthPrePassPipeline = CreateReloadablePipeline(desc, {{"DepthOnlyAnimated_vs", "DepthOnly_ps"}},
                                                                RsInVS);
        desc.CullingOff();
        doubleSideDepthPrePassPipeline = CreateReloadablePipeline(desc, {{"DepthOnly_vs", "DepthOnly_ps"}}, RsInVS);
        animatedDoubleSideDepthPrePassPipeline = CreateReloadablePipeline(desc, {
                                                                              {"DepthOnlyAnimated_vs", "DepthOnly_ps"}
                                                                          }, RsInVS);
    }

    // Shadow.
    {
        auto desc = CREATE_GRAPHICS_PSO
            .RenderTargets(nullptr, 0, shadowDepthFormat)
            .InputLayout(inputLayout_position)
            //.cullFrontFaces()
            ;

        shadowPipeline = CreateReloadablePipeline(desc, { "Shadow_vs" }, RsInVS);
        pointLightShadowPipeline = CreateReloadablePipeline(desc, { "ShadowPointLight_vs", "ShadowPointLight_ps" }, RsInVS);

        desc.CullingOff();
        doubleSidedShadowPipeline = CreateReloadablePipeline(desc, { "Shadow_vs" }, RsInVS);
        doubleSidedPointLightShadowPipeline = CreateReloadablePipeline(desc, { "ShadowPointLight_vs", "ShadowPointLight_ps" }, RsInVS);
    }
    // Shadow map copy.
    {
        auto desc = CREATE_GRAPHICS_PSO
            .RenderTargets(0, 0, shadowDepthFormat)
            .DepthSettings(true, true, D3D12_COMPARISON_FUNC_ALWAYS)
            .CullingOff();

        shadowMapCopyPipeline = CreateReloadablePipeline(desc, { "FullscreenTriangle_vs", "ShadowMapCopy_ps" });
    }
    worldSpaceFrustaPipeline = CreateReloadablePipeline("WorldSpaceTiledFrusta_cs");
    lightCullingPipeline = CreateReloadablePipeline("LightCulling_cs");

    LoadSsrShaders();
    LoadAoShaders();
    LoadAaShaders();

    hierarchicalLinearDepthPipeline = CreateReloadablePipeline("HierarchicalLinearDepth_cs");
    shadowBlurXPipeline = CreateReloadablePipeline("ShadowBlurX_cs");
    shadowBlurYPipeline = CreateReloadablePipeline("ShadowBlurY_cs");

    
}


void TextureSky(DxCommandList* cl, const DxRenderTarget& renderTarget, const mat4& proj, const mat4& view, ref<DxTexture> skyTextureCube, float intensity)
{
    PROFILE_ALL(cl, "sky");
    cl->SetRenderTarget(renderTarget);
    cl->SetViewport(renderTarget.viewport);
    cl->SetPipelineState(*textureSkyPipeline.pipelineStateObject);
    cl->SetGraphicsRootSignature(*textureSkyPipeline.rootSignature);

    cl->SetGraphics32BitConstants(SKY_RS_VP, SkyTransformCB{proj * CreateSkyViewMatrix(view)});
    cl->SetGraphics32BitConstants(SKY_RS_INTENSITY, intensity);
    cl->SetDescriptorHeapSRV(SKY_RS_TEX, 0, skyTextureCube);

    cl->DrawCubeTriangleStrip();
}


static void DepthPrePassInter(DxCommandList* cl,
                              DxPipeline& pipeline,
                              const SortKeyVector<float, StaticDepthOnlyRenderCommand>& staticCommands,
                              const SortKeyVector<float, DynamicDepthOnlyRenderCommand>& dynamicCommands,
                              const mat4& viewProj, const mat4& prevFrameViewProj,
                              DepthOnlyCameraJitterCB jitterCb)
{
    if(staticCommands.Size() > 0 || dynamicCommands.Size() > 0) {
        cl->SetPipelineState(*pipeline.pipelineStateObject);
        cl->SetGraphicsRootSignature(*pipeline.rootSignature);
        cl->SetGraphics32BitConstants(DEPTH_ONLY_RS_CAMERA_JITTER, jitterCb);
    }
    if(staticCommands.Size() > 0) {
        PROFILE_ALL(cl, "Static")
        for(const StaticDepthOnlyRenderCommand& dc : staticCommands) {
            cl->SetGraphics32BitConstants(DEPTH_ONLY_RS_OBJECT_ID, dc.objectId);
            cl->SetGraphics32BitConstants(DEPTH_ONLY_RS_MVP, DepthOnlyTransformCB{
                                             viewProj * dc.transform,
                                             prevFrameViewProj * dc.transform
                                         });
            cl->SetVertexBuffer(0, dc.vertexBufferView);
            cl->SetIndexBuffer(dc.indexBufferView);
            cl->DrawIndexed(dc.submeshInfo.numIndices, 1, dc.submeshInfo.firstIndex, dc.submeshInfo.baseVertex, 0);

        }
    }
    if (dynamicCommands.Size() > 0)
    {
        PROFILE_ALL(cl, "Dynamic");

        for (const DynamicDepthOnlyRenderCommand& dc : dynamicCommands)
        {
            cl->SetGraphics32BitConstants(DEPTH_ONLY_RS_OBJECT_ID, dc.objectId);
            cl->SetGraphics32BitConstants(DEPTH_ONLY_RS_MVP, DepthOnlyTransformCB{ viewProj * dc.transform, prevFrameViewProj * dc.prevFrameTransform });

            cl->SetVertexBuffer(0, dc.vertexBufferView);
            cl->SetIndexBuffer(dc.indexBuffer);
            cl->DrawIndexed(dc.submesh.numIndices, 1, dc.submesh.firstIndex, dc.submesh.baseVertex, 0);
        }
    }
}

static void DepthPrePassInter(DxCommandList* cl, const DxPipeline& pipeline,
                              const SortKeyVector<float, AnimatedDepthOnlyRenderCommand>& animatedCommands,
                              const mat4& viewProj, const mat4& prevFrameViewProj, const DepthOnlyCameraJitterCB& jitterCb)
{
    if(animatedCommands.Size() > 0) {
        PROFILE_ALL(cl, "Animated")

        cl->SetPipelineState(*pipeline.pipelineStateObject);
        cl->SetGraphicsRootSignature(*pipeline.rootSignature);
        cl->SetGraphics32BitConstants(DEPTH_ONLY_RS_CAMERA_JITTER, jitterCb);
        for(const AnimatedDepthOnlyRenderCommand& ac : animatedCommands) {
            cl->SetGraphics32BitConstants(DEPTH_ONLY_RS_MVP, DepthOnlyTransformCB{
                                             viewProj * ac.transform, prevFrameViewProj * ac.prevFrameTransform
                                         });
            cl->SetGraphics32BitConstants(DEPTH_ONLY_RS_OBJECT_ID, ac.objectId);
            cl->SetRootGraphicsSRV(
                DEPTH_ONLY_RS_PREV_FRAME_POSITIONS,
                ac.prevFrameVertexBufferAddress + ac.submeshInfo.baseVertex * ac.vertexBufferView.view.StrideInBytes);
            cl->SetVertexBuffer(0, ac.vertexBufferView);
            cl->SetIndexBuffer(ac.indexBufferView);
            cl->DrawIndexed(ac.submeshInfo.numIndices, 1, ac.submeshInfo.firstIndex, ac.submeshInfo.baseVertex, 0);


        }

    }
}

void DepthPrePass(DxCommandList* cl,
                  const DxRenderTarget& depthOnlyRenderTarget,
                  const OpaqueRenderPass* opaqueRenderPass,
                  const mat4& viewProj, const mat4& prevFrameViewProj,
                  vec2 jitter, vec2 preFrameJitter)
{
    if(opaqueRenderPass) {
        PROFILE_ALL(cl, "Depth pre-pass")

        cl->SetRenderTarget(depthOnlyRenderTarget);
        cl->SetViewport(depthOnlyRenderTarget.viewport);
        cl->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        DepthOnlyCameraJitterCB jitterCb = {jitter, preFrameJitter};

        DepthPrePassInter(cl, depthPrePassPipeline, opaqueRenderPass->staticDepthPrepass,
                          opaqueRenderPass->dynamicDepthPrepass, viewProj, prevFrameViewProj, jitterCb);
        DepthPrePassInter(cl, doubleSideDepthPrePassPipeline, opaqueRenderPass->staticDoubleSideDepthPrePass,
                          opaqueRenderPass->dynamicDoubleSideDepthPrepass, viewProj, prevFrameViewProj, jitterCb);

        DepthPrePassInter(cl, animatedDepthPrePassPipeline, opaqueRenderPass->animatedDepthPrePass, viewProj,
                          prevFrameViewProj, jitterCb);
        DepthPrePassInter(cl, animatedDepthPrePassPipeline, opaqueRenderPass->animatedDoubleSideDepthPrePass, viewProj,
                          prevFrameViewProj, jitterCb);
    }
}

void LinearDepthPyramid(DxCommandList* cl,
    ref<DxTexture> depthStencilBuffer,
    ref<DxTexture> linearDepthBuffer,
    vec4 projectionParams)
{
    PROFILE_ALL(cl, "Linear depth pyramid");

    cl->SetPipelineState(*hierarchicalLinearDepthPipeline.pipelineStateObject);
    cl->SetComputeRootSignature(*hierarchicalLinearDepthPipeline.rootSignature);

    float width = ceilf(depthStencilBuffer->width * 0.5f);
    float height = ceilf(depthStencilBuffer->height * 0.5f);

    cl->SetCompute32BitConstants(HIERARCHICAL_LINEAR_DEPTH_RS_CB, HierarchicalLinearDepthCB{ projectionParams, vec2(1.f / width, 1.f / height) });
    cl->SetDescriptorHeapUAV(HIERARCHICAL_LINEAR_DEPTH_RS_TEXTURES, 0, linearDepthBuffer->UavAt(0));
    cl->SetDescriptorHeapUAV(HIERARCHICAL_LINEAR_DEPTH_RS_TEXTURES, 1, linearDepthBuffer->UavAt(1));
    cl->SetDescriptorHeapUAV(HIERARCHICAL_LINEAR_DEPTH_RS_TEXTURES, 2, linearDepthBuffer->UavAt(2));
    cl->SetDescriptorHeapUAV(HIERARCHICAL_LINEAR_DEPTH_RS_TEXTURES, 3, linearDepthBuffer->UavAt(3));
    cl->SetDescriptorHeapUAV(HIERARCHICAL_LINEAR_DEPTH_RS_TEXTURES, 4, linearDepthBuffer->UavAt(4));
    cl->SetDescriptorHeapUAV(HIERARCHICAL_LINEAR_DEPTH_RS_TEXTURES, 5, linearDepthBuffer->UavAt(5));
    cl->SetDescriptorHeapSRV(HIERARCHICAL_LINEAR_DEPTH_RS_TEXTURES, 6, depthStencilBuffer);

    cl->Dispatch(bucketize((uint32)width, POST_PROCESSING_BLOCK_SIZE), bucketize((uint32)height, POST_PROCESSING_BLOCK_SIZE));
}


