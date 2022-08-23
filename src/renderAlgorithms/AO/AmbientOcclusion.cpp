#include "pch.h"
#include "rendering/RenderAlgorithms.h"

#include "PostProcessing_rs.hlsli"
#include "dx/DxBarrierBatcher.h"
#include "dx/DxProfiling.h"

void ambientOcclusion(DxCommandList* cl, ref<DxTexture> linearDepth, ref<DxTexture> screenVelocitiesTexture,
                      ref<DxTexture> aoCalculationTexture, ref<DxTexture> aoBlurTempTexture, ref<DxTexture> history,
                      ref<DxTexture> output, HBAO_Settings settings, DxDynamicConstantBuffer cameraCBV)
{
    PROFILE_ALL(cl, "HBAO");

    {
        PROFILE_ALL(cl, "Calculate AO");

        HBAO_CB cb;
        cb.screenWidth = aoCalculationTexture->width;
        cb.screenHeight = aoCalculationTexture->height;
        cb.depthBufferMipLevel = log2((float)linearDepth->width / aoCalculationTexture->width);
        cb.numRays = settings.numRays;
        cb.maxNumStepsPerRay = settings.maxNumStepsPerRay;
        cb.strength = settings.strength;
        cb.radius = settings.radius;
        cb.seed = (float)(dxContext.frameID & 0xFFFFFFFF);

        float alpha = 2 * M_PI / settings.numRays;
        cb.rayDeltaRotation = vec2(cos(alpha), sin(alpha));

        cl->SetPipelineState(*hbaoPipeline.pipelineStateObject);
        cl->SetComputeRootSignature(*hbaoPipeline.rootSignature);

        cl->SetDescriptorHeapUAV(HBAO_RS_TEXTURES, 0, aoCalculationTexture);
        cl->SetDescriptorHeapSRV(HBAO_RS_TEXTURES, 1, linearDepth);

        cl->SetCompute32BitConstants(HBAO_RS_CB, cb);
        cl->SetComputeDynamicConstantBuffer(HBAO_RS_CAMERA, cameraCBV);

        cl->Dispatch(bucketize(aoCalculationTexture->width, POST_PROCESSING_BLOCK_SIZE), bucketize(aoCalculationTexture->height, POST_PROCESSING_BLOCK_SIZE));

        BarrierBatcher(cl)
            //.uav(aoCalculationTexture)
            .Transition(aoCalculationTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    bilateralBlurShadows(cl, aoCalculationTexture, aoBlurTempTexture, linearDepth, screenVelocitiesTexture, history, output);
}