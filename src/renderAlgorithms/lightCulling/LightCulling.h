#pragma once
#include "dx/DxBuffer.h"
#include "pch.h"

#include "dx/DxCommandList.h"

struct LightCulling
{
    ref<DxBuffer> tiledWorldSpaceFrustaBuffer;      //outPut of worldSpaceTiledFrusta.hlsl; keep each tile information : LightCullingViewFrustum
    ref<DxBuffer> tiledCullingIndexCounter;
    ref<DxBuffer> tiledObjectsIndexBuffer;

    ref<DxTexture> tiledCullingGrid;        //numCullingTilesX * numCullingTilesY
    uint32 numCullingTilesX;
    uint32 numCullingTilesY;
    void AllocateIfNecessary(uint32 renderWidth, uint32 renderHeight);
};
static DxPipeline worldSpaceFrustaPipeline;
static DxPipeline lightCullingPipeline;

void LightAndDecalCulling(DxCommandList* cl, ref<DxTexture> depthStencilBuffer, ref<DxBuffer> pointLights,
                          ref<DxBuffer> spotLights,
                          ref<DxBuffer> decals,
                          LightCulling culling,
                          uint32 numPointLights, uint32 numSpotLights, uint32 numDecals,
                          DxDynamicConstantBuffer cameraCBV);