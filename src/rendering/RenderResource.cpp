#include "pch.h"
#include "RenderResources.h"
#include "RenderUtils.h"

void RenderResources::InitializeGlobalResources()
{
    {
        uint8 white[] = { 255, 255, 255, 255 };
        whiteTexture = CreateTexture(white, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
        SET_NAME(whiteTexture->resource, "White");
    }
    
    uint8 black[] = { 0, 0, 0, 255 };
    blackTexture = CreateTexture(black, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
    SET_NAME(blackTexture->resource, "Black");

    blackCubeTexture = CreateCubeTexture(black, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
    SET_NAME(blackCubeTexture->resource, "Black cube");

    noiseTexture = LoadTextureFromFile("assets/noise/blue_noise.dds", image_load_flags_noncolor); // Already compressed and in DDS format.

    shadowMap = CreateDepthTexture(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, shadowDepthFormat, 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    SET_NAME(shadowMap->resource, "Shadow map");

    staticShadowMapCache = CreateDepthTexture(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, shadowDepthFormat, 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    SET_NAME(staticShadowMapCache->resource, "Static shadow map cache");
}

