#pragma once
#include <dxgiformat.h>

static constexpr DXGI_FORMAT hdrFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
static constexpr DXGI_FORMAT ldrFormat = DXGI_FORMAT_R10G10B10A2_UNORM; //sRGB 

static constexpr DXGI_FORMAT screenVelocitiesFormat = DXGI_FORMAT_R16G16_FLOAT; //xy float only
static constexpr DXGI_FORMAT worldNormalsRoughnessFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; // Alpha channel not used right now.
static constexpr DXGI_FORMAT linearDepthFormat = DXGI_FORMAT_R32_FLOAT;

static constexpr DXGI_FORMAT objectIDsFormat = DXGI_FORMAT_R32_UINT; //uint32 objectID
static constexpr DXGI_FORMAT skyPassFormats[] = {hdrFormat, screenVelocitiesFormat, objectIDsFormat};
static constexpr DXGI_FORMAT depthOnlyFormats[] = {screenVelocitiesFormat, objectIDsFormat};
static constexpr DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

static constexpr DXGI_FORMAT shadowDepthFormat = DXGI_FORMAT_D16_UNORM;

static constexpr DXGI_FORMAT aoFormat = DXGI_FORMAT_R8_UNORM;
static constexpr DXGI_FORMAT sssFormat = DXGI_FORMAT_R8_UNORM;

enum color_depth
{
    color_depth_8,
    color_depth_10,
};
static constexpr DXGI_FORMAT ColorDepthToFormat(color_depth colorDepth)
{
    return (colorDepth == color_depth_8) ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R10G10B10A2_UNORM;
}


void WaitForSkinningToFinish();
void InitializeRenderUtils();
void FrameCommon();