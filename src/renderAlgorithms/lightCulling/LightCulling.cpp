#include "pch.h"
#include "LightCulling.h"
#include "core/math.h"
#include  "rendering/RenderAlgorithms.h"
#include "LightCulling_rs.hlsli"
#include "dx/DxBarrierBatcher.h"
#include "dx/DxProfiling.h"
#include "rendering/RenderResources.h"

void LightCulling::AllocateIfNecessary(uint32 renderWidth, uint32 renderHeight)
{
    uint32 oldNumCullingTilesX = numCullingTilesX;
    uint32 oldNumcullingTilesY = numCullingTilesY;

    numCullingTilesX = bucketize(renderWidth, LIGHT_CULLING_TILE_SIZE);
    numCullingTilesY = bucketize(renderHeight, LIGHT_CULLING_TILE_SIZE);
    bool firstAllocation = (tiledCullingGrid == nullptr);
    if(firstAllocation) {
        tiledCullingGrid = CreateTexture(nullptr, numCullingTilesX, numCullingTilesY, DXGI_FORMAT_R32G32B32A32_UINT, false,
                                         false, true);
        tiledCullingIndexCounter = CreateBuffer(sizeof(uint32), 1, nullptr, true, true);
        tiledObjectsIndexBuffer = CreateBuffer(sizeof(uint32),
                                               numCullingTilesX * numCullingTilesY * MAX_NUM_INDICES_PER_TILE *
                                               NUM_BUFFERED_FRAMES, nullptr,
                                               true, false); //per frame;
        tiledWorldSpaceFrustaBuffer = CreateBuffer(sizeof(LightCullingViewFrustum), numCullingTilesX * numCullingTilesY, 0,
                                                   true);

        SET_NAME(tiledCullingGrid->resource, "Tiled culling grid");
        SET_NAME(tiledCullingIndexCounter->resource, "Tiled index counter");
        SET_NAME(tiledObjectsIndexBuffer->resource, "Tiled index list");
        SET_NAME(tiledWorldSpaceFrustaBuffer->resource, "Tiled frusta");
    } else {
        if(numCullingTilesX != oldNumCullingTilesX || numCullingTilesY != oldNumcullingTilesY) {
            ResizeTexture(tiledCullingGrid, numCullingTilesX, numCullingTilesY);
            ResizeBuffer(tiledObjectsIndexBuffer,
                         numCullingTilesX * numCullingTilesY * MAX_NUM_LIGHTS_PER_TILE * NUM_BUFFERED_FRAMES);
            ResizeBuffer(tiledWorldSpaceFrustaBuffer, numCullingTilesX * numCullingTilesY);
            // counter fix to 1
        }
    }
}

void LightAndDecalCulling(DxCommandList* cl, ref<DxTexture> depthStencilBuffer, ref<DxTexture> pointLights,
                          ref<DxTexture> spotLights, ref<DxBuffer> decals, LightCulling culling, uint32 numPointLights,
                          uint32 numSpotLights, uint32 numDecals, DxDynamicConstantBuffer cameraCBV)
{
    if(numPointLights || numSpotLights || numDecals) {
        PROFILE_ALL(cl, "cull lights & decals");
        {
            cl->SetPipelineState(*worldSpaceFrustaPipeline.pipelineStateObject);
            cl->SetComputeRootSignature(*worldSpaceFrustaPipeline.rootSignature);
            cl->SetComputeDynamicConstantBuffer(WORLD_SPACE_TILED_FRUSTA_RS_CAMERA, cameraCBV);
            cl->SetCompute32BitConstants(WORLD_SPACE_TILED_FRUSTA_RS_CB, FrustaCB{culling.numCullingTilesX, culling.numCullingTilesY}); //bucketize(renderWidth, LIGHT_CULLING_TILE_SIZE);
            cl->SetRootComputeUAV(WORLD_SPACE_TILED_FRUSTA_RS_FRUSTA_UAV, culling.tiledWorldSpaceFrustaBuffer);
            cl->Dispatch(bucketize(culling.numCullingTilesX, 16), bucketize(culling.numCullingTilesY, 16));
        }
        BarrierBatcher(cl).Uav(culling.tiledWorldSpaceFrustaBuffer);

        {
            cl->ClearUAV(culling.tiledCullingIndexCounter, 0.f);
            cl->SetPipelineState(*lightCullingPipeline.pipelineStateObject);
            cl->SetComputeRootSignature(*lightCullingPipeline.rootSignature);

            cl->SetComputeDynamicConstantBuffer(LIGHT_CULLING_RS_CAMERA, cameraCBV);
            cl->SetCompute32BitConstants(LIGHT_CULLING_RS_CB, LightCullingCB{ culling.numCullingTilesX, numPointLights, numSpotLights, numDecals });
            cl->SetDescriptorHeapSRV(LIGHT_CULLING_RS_SRV_UAV, 0, depthStencilBuffer);
            cl->SetDescriptorHeapSRV(LIGHT_CULLING_RS_SRV_UAV, 1, culling.tiledWorldSpaceFrustaBuffer);
            cl->SetDescriptorHeapSRV(LIGHT_CULLING_RS_SRV_UAV, 2, pointLights ? pointLights->defaultSRV : RenderResources::nullBufferSRV);
            cl->SetDescriptorHeapSRV(LIGHT_CULLING_RS_SRV_UAV, 3, spotLights ? spotLights->defaultSRV : RenderResources::nullBufferSRV);
            cl->SetDescriptorHeapSRV(LIGHT_CULLING_RS_SRV_UAV, 4, decals ? decals->defaultSRV : RenderResources::nullBufferSRV);
            cl->SetDescriptorHeapSRV(LIGHT_CULLING_RS_SRV_UAV, 5, culling.tiledCullingGrid);
            cl->SetDescriptorHeapSRV(LIGHT_CULLING_RS_SRV_UAV, 6, culling.tiledCullingIndexCounter);
            cl->SetDescriptorHeapSRV(LIGHT_CULLING_RS_SRV_UAV, 7, culling.tiledObjectsIndexBuffer);
            cl->Dispatch(culling.numCullingTilesX, culling.numCullingTilesY);
        }
        BarrierBatcher(cl)
            .Uav(culling.tiledCullingGrid)
            .Uav(culling.tiledObjectsIndexBuffer);
    }
}