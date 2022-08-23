#pragma once
#include "dx/DxTexture.h"
#define SHADOW_MAP_WIDTH 6144
#define SHADOW_MAP_HEIGHT 6144

struct RenderResources
{
    static ref<DxTexture> whiteTexture;
    static ref<DxTexture> blackTexture;
    static ref<DxTexture> blackCubeTexture;
    static ref<DxTexture> noiseTexture;

    static DxCpuDescriptorHandle nullTextureSRV;
    static DxCpuDescriptorHandle nullBufferSRV;
    static DxRtvDescriptorHandle nullScreenVelocitiesRTV;
    static DxRtvDescriptorHandle nullObjectIDsRTV;

    static ref<DxTexture> shadowMap;
    static ref<DxTexture> staticShadowMapCache;
    static void InitializeGlobalResources();
    
    
};
