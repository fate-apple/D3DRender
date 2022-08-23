#pragma once
#include "Dx.h"
#include "DxDescriptorAllocation.h"
#include "core/Asset.h"
#include "core/Image.h"

struct DxTexture
{
    Dx12Resource resource;
    D3D12MA::Allocation* allocation = nullptr;

    DxDescriptorAllocation srvUavAllocation{};
    DxDescriptorAllocation rtvAllocation{};
    DxDescriptorAllocation dsvAllocation{};

    DxCpuDescriptorHandle defaultSRV; // SRV for the whole texture (all mip levels).
    DxCpuDescriptorHandle defaultUAV; // UAV for the first mip level.
    DxCpuDescriptorHandle UavAt(uint32 index) { return srvUavAllocation.CpuAt(1 + index); }

    DxCpuDescriptorHandle stencilSRV;
    DxRtvDescriptorHandle defaultRTV;
    DxRtvDescriptorHandle RtvAt(uint32 index) { return rtvAllocation.CpuAt(index); }
    DxDsvDescriptorHandle defaultDSV;

    uint32 width, height;
    uint16 depth;
    DXGI_FORMAT format;
    D3D12_RESOURCE_STATES initialState;

    bool supportRTV;
    bool supportDSV;
    bool supportSRV;
    bool supportUAV;

    uint16 requestedNumMipLevels;
    uint32 numMipLevels;

    AssetHandle assetHandle;

    void SetName(const wchar* name);
    std::wstring GetName();
};


struct TextureGrave
{
    Dx12Resource resource;

    DxDescriptorAllocation srvUavAllocation = {};
    DxDescriptorAllocation rtvAllocation = {};
    DxDescriptorAllocation dsvAllocation = {};

    TextureGrave() = default;
    TextureGrave(const TextureGrave& other) = delete;
    TextureGrave(TextureGrave&& other) = default;
    TextureGrave& operator=(const TextureGrave& other) = delete;
    TextureGrave& operator=(TextureGrave&& other) = default;
    ~TextureGrave();
};

static bool IsDepthFormat(DXGI_FORMAT format)
{
    switch(format) {
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_D16_UNORM:
        return true;
    default:
        return false;
    }
}

static bool IsStencilFormat(DXGI_FORMAT format)
{
    switch(format) {
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return true;
    default:
        return false;
    }
}

void ResizeTexture(ref<DxTexture> texture, uint32 width, uint32 height,
                   D3D12_RESOURCE_STATES initialState = static_cast<D3D12_RESOURCE_STATES>(-1));
void UploadTextureSubresourceData(ref<DxTexture> texture, D3D12_SUBRESOURCE_DATA* subresourceData, uint32 firstSubresourceIndex,
                                  uint32 numSubresources);
ref<DxTexture> CreateTexture(D3D12_RESOURCE_DESC desc, D3D12_SUBRESOURCE_DATA* subresourceData, uint32 numSubresouces,
                             D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON, bool mipUavs = false);
ref<DxTexture> CreateTexture(const void* data, uint32 width, uint32 height, DXGI_FORMAT format, bool allocateMips = false,
                             bool allowRenderTarget = false, bool allowUnorderedAccess = false,
                             D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON, bool mipUavs = false);
ref<DxTexture> CreateCubeTexture(const void* data, uint32 width, uint32 height, DXGI_FORMAT format,
                                 bool allocateMipMaps = false, bool allowRenderTarget = false,
                                 bool allowUnorderedAccess = false,
                                 D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON, bool mipUAVs = false);
ref<DxTexture> CreateDepthTexture(uint32 width, uint32 height, DXGI_FORMAT format, uint32 arrayLength = 1, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE);

ref<DxTexture> LoadTextureFromFile(const fs::path& filename, uint32 flags = image_load_flags_default);
ref<DxTexture> LoadTextureFromMemory(const void* ptr, uint32 size, image_format format, const fs::path& cachePath,
                                     uint32 flags = image_load_flags_default);