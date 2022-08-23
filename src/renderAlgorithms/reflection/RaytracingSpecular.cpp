#include "pch.h"
#include "RaytracingSpecular.h"

#include "dx/DxBarrierBatcher.h"
#include "dx/DxProfiling.h"
#include "rendering/RenderResources.h"

// mainly https://www.ea.com/frostbite/news/stochastic-screen-space-reflections
void ScreenSpaceReflections(DxCommandList* cl, ref<DxTexture> raycastTexture, ref<DxTexture> depthStencilBuffer,
                            ref<DxTexture> linearDepthBuffer, ref<DxTexture> worldNormalsRoughnessTexture,
                            ref<DxTexture> screenVelocitiesTexture, ref<DxTexture> prevFrameHdr, ref<DxTexture> resolveTexture,
                            ref<DxTexture> ssrTemporalHistory, ref<DxTexture> ssrTemporalOutput, SSR_Settings settings,
                            DxDynamicConstantBuffer cameraCBV)
{
    PROFILE_ALL(cl, "Screen space reflections");

    uint32 raycastWidth = raycastTexture->width;
    uint32 raycastHeight = raycastTexture->height;

    uint32 resolveWidth = resolveTexture->width;
    uint32 resolveHeight = resolveTexture->height;

    {
        PROFILE_ALL(cl, "Ray cast");
        cl->SetPipelineState(*ssrRaycastPipeline.pipelineStateObject);
        cl->SetComputeRootSignature(*ssrRaycastPipeline.rootSignature);

        SSR_RaycastCB raycastSettings;
        raycastSettings.numSteps = settings.numSteps;
        raycastSettings.maxDistance = settings.maxDistance;
        raycastSettings.strideCutOff = settings.strideCutoff;
        raycastSettings.minStride = settings.minStride;
        raycastSettings.maxStride = settings.maxStride;
        raycastSettings.dimensions = vec2((float)raycastWidth, (float)raycastHeight);
        raycastSettings.invDimensions = vec2(1.f / raycastWidth, 1.f / raycastHeight);
        raycastSettings.frameIndex = (uint32)dxContext.frameID;

        cl->SetCompute32BitConstants(SSR_RAYCAST_RS_CB, raycastSettings);
        cl->SetComputeDynamicConstantBuffer(SSR_RAYCAST_RS_CAMERA, cameraCBV);
        cl->SetDescriptorHeapUAV(SSR_RAYCAST_RS_TEXTURES, 0, raycastTexture);
        cl->SetDescriptorHeapSRV(SSR_RAYCAST_RS_TEXTURES, 1, depthStencilBuffer);
        cl->SetDescriptorHeapSRV(SSR_RAYCAST_RS_TEXTURES, 2, linearDepthBuffer);
        cl->SetDescriptorHeapSRV(SSR_RAYCAST_RS_TEXTURES, 3, worldNormalsRoughnessTexture);
        cl->SetDescriptorHeapSRV(SSR_RAYCAST_RS_TEXTURES, 4, RenderResources::noiseTexture);
        cl->SetDescriptorHeapSRV(SSR_RAYCAST_RS_TEXTURES, 5, screenVelocitiesTexture);
        cl->Dispatch(bucketize(raycastWidth, SSR_BLOCK_SIZE), bucketize(raycastHeight, SSR_BLOCK_SIZE));

        BarrierBatcher(cl)
        //.uav(raycastTexture)
        .Transition(raycastTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    {
        PROFILE_ALL(cl, "Resolve");
        cl->SetPipelineState(*ssrResolvePipeline.pipelineStateObject);
        cl->SetComputeRootSignature(*ssrResolvePipeline.rootSignature);
        cl->SetCompute32BitConstants(SSR_RESOLVE_RS_CB, SSR_ResolveCB{
                                         vec2((float)resolveWidth, (float)resolveHeight),
                                         vec2(1.f / resolveWidth, 1.f / resolveHeight)
                                     });
        cl->SetComputeDynamicConstantBuffer(SSR_RESOLVE_RS_CAMERA, cameraCBV);

        cl->SetDescriptorHeapUAV(SSR_RESOLVE_RS_TEXTURES, 0, resolveTexture);
        cl->SetDescriptorHeapSRV(SSR_RESOLVE_RS_TEXTURES, 1, depthStencilBuffer);
        cl->SetDescriptorHeapSRV(SSR_RESOLVE_RS_TEXTURES, 2, worldNormalsRoughnessTexture);
        cl->SetDescriptorHeapSRV(SSR_RESOLVE_RS_TEXTURES, 3, raycastTexture);
        cl->SetDescriptorHeapSRV(SSR_RESOLVE_RS_TEXTURES, 4, prevFrameHdr);
        cl->SetDescriptorHeapSRV(SSR_RESOLVE_RS_TEXTURES, 5, screenVelocitiesTexture);

        cl->Dispatch(bucketize(resolveWidth, SSR_BLOCK_SIZE), bucketize(resolveHeight, SSR_BLOCK_SIZE));
        BarrierBatcher(cl)
            //.uav(resolveTexture)
            .Transition(resolveTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
            .Transition(raycastTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    {
        PROFILE_ALL(cl, "Temporal");
	
        cl->SetPipelineState(*ssrTemporalPipeline.pipelineStateObject);
        cl->SetComputeRootSignature(*ssrTemporalPipeline.rootSignature);
	
        cl->SetCompute32BitConstants(SSR_TEMPORAL_RS_CB, SSR_TemporalCB{ vec2(1.f / resolveWidth, 1.f / resolveHeight) });
	
        cl->SetDescriptorHeapUAV(SSR_TEMPORAL_RS_TEXTURES, 0, ssrTemporalOutput);           
        cl->SetDescriptorHeapSRV(SSR_TEMPORAL_RS_TEXTURES, 1, resolveTexture);
        cl->SetDescriptorHeapSRV(SSR_TEMPORAL_RS_TEXTURES, 2, ssrTemporalHistory);
        cl->SetDescriptorHeapSRV(SSR_TEMPORAL_RS_TEXTURES, 3, screenVelocitiesTexture);         //(prevNdc - ndc)
	
        cl->Dispatch(bucketize(resolveWidth, SSR_BLOCK_SIZE), bucketize(resolveHeight, SSR_BLOCK_SIZE));
	
        BarrierBatcher(cl)
            //.uav(ssrOutput)
            .Transition(ssrTemporalOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
            .Transition(ssrTemporalHistory, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            .Transition(resolveTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }
    {
        PROFILE_ALL(cl, "Median Blur");
	
        cl->SetPipelineState(*ssrMedianBlurPipeline.pipelineStateObject);
        cl->SetComputeRootSignature(*ssrMedianBlurPipeline.rootSignature);
	
        cl->SetCompute32BitConstants(SSR_MEDIAN_BLUR_RS_CB, SSR_MedianBlurCB{ vec2(1.f / resolveWidth, 1.f / resolveHeight) });
	
        cl->SetDescriptorHeapUAV(SSR_MEDIAN_BLUR_RS_TEXTURES, 0, resolveTexture);           // We reuse the resolve texture here.
        cl->SetDescriptorHeapSRV(SSR_MEDIAN_BLUR_RS_TEXTURES, 1, ssrTemporalOutput);        // result after Temporal blend
	
        cl->Dispatch(bucketize(resolveWidth, SSR_BLOCK_SIZE), bucketize(resolveHeight, SSR_BLOCK_SIZE));
	
        BarrierBatcher(cl)
            //.uav(resolveTexture)
            .Transition(resolveTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
    }
}