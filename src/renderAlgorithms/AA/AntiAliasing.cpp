#include "pch.h"
#include "AntiAliasing.h"

#include "PostProcessing_rs.hlsli"
#include "dx/DxBarrierBatcher.h"
#include "dx/DxProfiling.h"


void temporalAntiAliasing(DxCommandList* cl, ref<DxTexture> hdrInput, ref<DxTexture> screenVelocitiesTexture,
                          ref<DxTexture> depthStencilBuffer, ref<DxTexture> history, ref<DxTexture> output,
                          vec4 jitteredCameraProjectionParams)
{
    PROFILE_ALL(cl, "Temporal anti-aliasing");

    cl->SetPipelineState(*taaPipeline.pipelineStateObject);
    cl->SetComputeRootSignature(*taaPipeline.rootSignature);

    cl->SetDescriptorHeapUAV(TAA_RS_TEXTURES, 0, output);
    cl->SetDescriptorHeapSRV(TAA_RS_TEXTURES, 1, hdrInput);
    cl->SetDescriptorHeapSRV(TAA_RS_TEXTURES, 2, history);
    cl->SetDescriptorHeapSRV(TAA_RS_TEXTURES, 3, screenVelocitiesTexture);
    cl->SetDescriptorHeapSRV(TAA_RS_TEXTURES, 4, depthStencilBuffer);

    uint32 renderWidth = depthStencilBuffer->width;
    uint32 renderHeight = depthStencilBuffer->height;
    cl->SetCompute32BitConstants(TAA_RS_CB, TAA_CB{ jitteredCameraProjectionParams, vec2((float)renderWidth, (float)renderHeight) });

    cl->Dispatch(bucketize(renderWidth, POST_PROCESSING_BLOCK_SIZE), bucketize(renderHeight, POST_PROCESSING_BLOCK_SIZE));

    BarrierBatcher(cl)
        //.uav(taaTextures[taaOutputIndex])
        .Transition(output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) // Will be read by rest of post processing stack. Can stay in read state, since it is read as history next frame.
        .Transition(history, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS); // Will be used as UAV next frame.
}