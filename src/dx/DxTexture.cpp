#include "pch.h"
#include "DxTexture.h"

#include "DxContext.h"
#include "core/Image.h"

static bool CheckFormatSupport(D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport, D3D12_FORMAT_SUPPORT1 support1)
{
    return (formatSupport.Support1 & support1) != 0;
}
static bool CheckFormatSupport(D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport, D3D12_FORMAT_SUPPORT2 support2)
{
    return (formatSupport.Support2 & support2) != 0;
}

static bool FormatSupportsRtv(D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport)
{
    return CheckFormatSupport(formatSupport, D3D12_FORMAT_SUPPORT1_RENDER_TARGET);
}

static bool FormatSupportsDsv(D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport)
{
    return CheckFormatSupport(formatSupport, D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL);
}

static bool FormatSupportsSrv(D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport)
{
    return CheckFormatSupport(formatSupport, D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE);
}

static bool FormatSupportsUav(D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport)
{
    return CheckFormatSupport(formatSupport, D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
    CheckFormatSupport(formatSupport, D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) &&
    CheckFormatSupport(formatSupport, D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
}

static ref<DxTexture> UploadImageToGpu(DirectX::ScratchImage& scratchImage, D3D12_RESOURCE_DESC& desc, uint32 flags)
{
    const DirectX::Image* images = scratchImage.GetImages();
    uint32 numImages = (uint32)scratchImage.GetImageCount();

    D3D12_SUBRESOURCE_DATA subresources[128]{};
    for(uint32 i(0); i < numImages; ++i) {
        D3D12_SUBRESOURCE_DATA& subresourceData = subresources[i];
        subresourceData.RowPitch = images[i].rowPitch;
        subresourceData.SlicePitch = images[i].slicePitch;
        subresourceData.pData = images[i].pixels;
    }
    ref<DxTexture> result = CreateTexture(desc, subresources, numImages);
    SET_NAME(result->resource, "Loaded from file");

    if(flags & image_load_flags_gen_mips_on_gpu) {
        dxContext.renderQueue.WaitForOtherQueue(dxContext.copyQueue);
        DxCommandList* cl = dxContext.GetFreeRenderCommandList();
        GenerateMipMapsOnGPU(cl, result);
        dxContext.ExecuteCommandList(cl);
    }

    return result;
}

static ref<DxTexture> LoadTextureInternal(const fs::path& path, uint32 flags)
{
    if(flags & image_load_flags_gen_mips_on_gpu) {
        flags &= ~image_load_flags_gen_mips_on_cpu;
        flags |= image_load_flags_allocate_full_mipchain;
    }

    DirectX::ScratchImage scratchImage;
    D3D12_RESOURCE_DESC textureDesc;

    if(path.extension() == ".svg") {
        if(!loadSVGFromFile(path, flags, scratchImage, textureDesc)) {
            return nullptr;
        }
    } else if(!loadImageFromFile(path, flags, scratchImage, textureDesc)) {
        return nullptr;
    }

    return UploadImageToGpu(scratchImage, textureDesc, flags);
}

TextureGrave::~TextureGrave()
{
    wchar name[128];

    if(resource) {
        uint32 size = sizeof(name);
        resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name);
        name[(std::min)(static_cast<uint32>(arraySize(name)) - 1, size)] = 0;
        // [C2589] “(”:“::”右边的非法标记. macro conflict. solved: add brace 

        dxContext.srvUavHeap.Free(srvUavAllocation);
        dxContext.rtvHeap.Free(rtvAllocation);
        dxContext.dsvHeap.Free(dsvAllocation);
    }
}

static void Retire(Dx12Resource resource, DxDescriptorAllocation srvUavAllocation, DxDescriptorAllocation rtvAllocation,
                   DxDescriptorAllocation dsvAllocation)
{
    TextureGrave grave;
    grave.resource = resource;
    grave.srvUavAllocation = srvUavAllocation;
    grave.rtvAllocation = rtvAllocation;
    grave.dsvAllocation = dsvAllocation;
    dxContext.Retire(std::move(grave));
}

void ResizeTexture(ref<DxTexture> texture, uint32 width, uint32 height, D3D12_RESOURCE_STATES initialState)
{
    if(!texture) return;
    wchar name[128];
    uint32 size = sizeof(name);
    texture->resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name);
    name[min((uint32)arraySize(name) - 1, size)] = 0;

    bool hasMipUAVs = texture->srvUavAllocation.count > 2;
    Retire(texture->resource, texture->srvUavAllocation, texture->rtvAllocation, texture->dsvAllocation);
    if(texture->allocation) dxContext.Retire(texture->allocation);
    D3D12_RESOURCE_DESC desc = texture->resource->GetDesc();
    texture->resource.Reset();

    D3D12_RESOURCE_STATES state = (initialState == -1) ? texture->initialState : initialState;
    D3D12_CLEAR_VALUE* clearValue = nullptr;
    D3D12_CLEAR_VALUE optimizedClearValue{};
    if(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) {
        optimizedClearValue.Format = texture->format;
        optimizedClearValue.DepthStencil = {1, 0};
        clearValue = &optimizedClearValue;
    }
    uint32 maxNumMipLevels = static_cast<uint32>(log2(static_cast<float>(max(width, height)))) + 1;
    desc.MipLevels = min(maxNumMipLevels, static_cast<uint32>(texture->requestedNumMipLevels));
    desc.Width = width;
    desc.Height = height;
    texture->width = width;
    texture->height = height;

    auto heapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    CheckResult(dxContext.device->CreateCommittedResource(
        &heapDesc,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        state,
        clearValue,
        IID_PPV_ARGS(&texture->resource)));
    texture->numMipLevels = texture->resource->GetDesc().MipLevels;
    uint32 numUavMips = 0;
    if(texture->supportUAV) {
        numUavMips = (hasMipUAVs ? texture->numMipLevels : 1); //just follow before;
    }
    // rtv
    if(texture->supportRTV) {
        texture->rtvAllocation = dxContext.rtvHeap.Allocate(1);
        texture->defaultRTV = DxRtvDescriptorHandle(texture->rtvAllocation.CpuAt(0)).Create2DTextureRTV(texture);
    }
    //dsv & srv
    else if(texture->supportDSV) {
        texture->dsvAllocation = dxContext.dsvHeap.Allocate(1);
        texture->defaultDSV = DxDsvDescriptorHandle(texture->dsvAllocation.CpuAt(0)).Create2DTextureDSV(texture);
        texture->srvUavAllocation = dxContext.srvUavHeap.Allocate(1 + IsStencilFormat(texture->format));
        if(texture->depth == 1) {
            texture->defaultSRV = DxCpuDescriptorHandle(texture->srvUavAllocation.CpuAt(0)).CreateDepthTextureSRV(texture);
        } else {
            texture->defaultSRV = DxCpuDescriptorHandle(texture->srvUavAllocation.CpuAt(0)).CreateDepthTextureArraySRV(texture);
        }
        if(IsStencilFormat(texture->format)) {
            texture->stencilSRV = DxCpuDescriptorHandle(texture->srvUavAllocation.CpuAt(1)).CreateStencilTextureSRV(texture);
        }
    } else {
        texture->srvUavAllocation = dxContext.srvUavHeap.Allocate(1 + numUavMips);
        if(desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
            texture->defaultSRV = DxCpuDescriptorHandle(texture->srvUavAllocation.CpuAt(0)).CreateVolumeTextureSRV(texture);
        } else if(texture->depth == 6) {
            texture->defaultSRV = DxCpuDescriptorHandle(texture->srvUavAllocation.CpuAt(0)).CreateCubemapSRV(texture);
        } else {
            texture->defaultSRV = DxCpuDescriptorHandle(texture->srvUavAllocation.CpuAt(0)).Create2DTextureSRV(texture);
        }

        if(texture->supportUAV) {
            texture->defaultUAV = texture->srvUavAllocation.CpuAt(1);
            for(uint32 i(0); i < numUavMips; ++i) {
                if(desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
                    DxCpuDescriptorHandle(texture->srvUavAllocation.CpuAt(1 + i)).CreateVolumeTextureUAV(texture);
                } else if(texture->depth == 6) {
                    DxCpuDescriptorHandle(texture->srvUavAllocation.CpuAt(1 + i)).CreateCubemapUAV(texture);
                } else {
                    DxCpuDescriptorHandle(texture->srvUavAllocation.CpuAt(1 + i)).Create2DTextureUAV(texture);
                }
            }
        }

    }
}

void UploadTextureSubresourceData(ref<DxTexture> texture, D3D12_SUBRESOURCE_DATA* subresourceData,
                                  uint32 firstSubresourceIndex, uint32 numSubresources)
{
    DxCommandList* cl = dxContext.GetFreeCopyCommandList();
    cl->TransitionBarrier(texture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    uint64 requiredSize = GetRequiredIntermediateSize(texture->resource.Get(), firstSubresourceIndex, numSubresources);
    Dx12Resource intermediateResource;
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);
    auto heapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CheckResult(dxContext.device->CreateCommittedResource(
        &heapDesc,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        0,
        IID_PPV_ARGS(&intermediateResource)));
    UpdateSubresources<128>(cl->commandList.Get(), texture->resource.Get(), intermediateResource.Get(), 0,
                            firstSubresourceIndex, numSubresources, subresourceData);
    dxContext.Retire(intermediateResource);
    dxContext.ExecuteCommandList(cl);
}

/**
 * \brief read flags & CheckFeatureSupport-> CreateCommittedResource -> UploadTextureSubresourceData -> create descriptor handle
 */
ref<DxTexture> CreateTexture(D3D12_RESOURCE_DESC desc, D3D12_SUBRESOURCE_DATA* subresourceData, uint32 numSubresouces,
                             D3D12_RESOURCE_STATES initialState, bool mipUavs)
{
    ref<DxTexture> result = makeRef<DxTexture>();
    result->requestedNumMipLevels = desc.MipLevels;
    uint16 maxNumMipLevels = static_cast<uint16>(log2(static_cast<float>(max(desc.Width, (UINT64)desc.Height)))) + 1;
    desc.MipLevels = min(maxNumMipLevels, result->requestedNumMipLevels);

    D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport;
    formatSupport.Format = desc.Format;
    CheckResult(dxContext.device->CheckFeatureSupport(
        D3D12_FEATURE_FORMAT_SUPPORT,
        &formatSupport, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
    result->supportRTV = (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) && FormatSupportsRtv(formatSupport);
    result->supportDSV = (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) && FormatSupportsDsv(formatSupport);
    result->supportUAV = (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) && FormatSupportsUav(formatSupport);
    result->supportSRV = FormatSupportsSrv(formatSupport);

    auto heapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    CheckResult(dxContext.device->CreateCommittedResource(&heapDesc,
                                                          D3D12_HEAP_FLAG_NONE,
                                                          &desc,
                                                          initialState, 0,
                                                          IID_PPV_ARGS(&result->resource)));
    result->numMipLevels = result->resource->GetDesc().MipLevels;
    result->format = desc.Format;
    result->width = static_cast<uint32>(desc.Width);
    result->height = desc.Height;
    result->depth = desc.DepthOrArraySize;
    result->initialState = initialState;

    if(subresourceData) {
        UploadTextureSubresourceData(result, subresourceData, 0, numSubresouces);
    }
    uint32 numUAVMips = 0;
    if(result->supportUAV) {
        numUAVMips = (mipUavs ? result->numMipLevels : 1);
    }
    result->srvUavAllocation = dxContext.srvUavHeap.Allocate(1 + numUAVMips);

    if(desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        result->defaultSRV = DxCpuDescriptorHandle(result->srvUavAllocation.CpuAt(0)).CreateVolumeTextureSRV(result);
    } else if(desc.DepthOrArraySize == 6) {
        result->defaultSRV = DxCpuDescriptorHandle(result->srvUavAllocation.CpuAt(0)).CreateCubemapSRV(result);
    } else {
        result->defaultSRV = DxCpuDescriptorHandle(result->srvUavAllocation.CpuAt(0)).Create2DTextureSRV(result);
    }

    if(result->supportRTV) {
        result->rtvAllocation = dxContext.rtvHeap.Allocate(desc.DepthOrArraySize);
        for(uint32 i(0); i < desc.DepthOrArraySize; ++i) {
            DxRtvDescriptorHandle(result->rtvAllocation.CpuAt(i)).Create2DTextureRTV(result, i);
        }
        result->defaultRTV = result->rtvAllocation.CpuAt(0);
    }

    if(result->supportUAV) {
        result->defaultUAV = result->srvUavAllocation.CpuAt(1);
        for(uint32 i(0); i < numUAVMips; ++i) {
            if(desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
                DxCpuDescriptorHandle(result->srvUavAllocation.CpuAt(1 + i)).CreateVolumeTextureUAV(result, i);
            } else if(desc.DepthOrArraySize == 6) {
                DxCpuDescriptorHandle(result->srvUavAllocation.CpuAt(1 + i)).CreateCubemapUAV(result, i);
            } else {
                DxCpuDescriptorHandle(result->srvUavAllocation.CpuAt(1 + i)).Create2DTextureUAV(result, i);
            }
        }
    }
    return result;
}

ref<DxTexture> CreateTexture(const void* data, uint32 width, uint32 height, DXGI_FORMAT format, bool allocateMips,
                             bool allowRenderTarget, bool allowUnorderedAccess, D3D12_RESOURCE_STATES initialState,
                             bool mipUavs)
{
    //TODO: sjw. remove mipUavs? 
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
    | (allowRenderTarget ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE)
    | (allowUnorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE);
    uint32 numMips = allocateMips ? 0 : 1;
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, numMips, 1, 0, flags);
    if(data) {
        uint32 formatSize = GetFormatSize(desc.Format);
        D3D12_SUBRESOURCE_DATA subresourceData;
        subresourceData.RowPitch = width * formatSize;
        subresourceData.SlicePitch = width * height * formatSize;
        subresourceData.pData = data;
        return CreateTexture(desc, &subresourceData, 1, initialState, mipUavs);
    }
    return CreateTexture(desc, nullptr, 0, initialState, mipUavs);
}

ref<DxTexture> CreateCubeTexture(const void* data, uint32 width, uint32 height, DXGI_FORMAT format, bool allocateMipMaps,
                                 bool allowRenderTarget, bool allowUnorderedAccess, D3D12_RESOURCE_STATES initialState,
                                 bool mipUAVs)
{
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
    | (allowRenderTarget ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE)
    | (allowUnorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE);
    uint32 numMips = allocateMipMaps ? 0 : 1;
    CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 6, numMips, 1, 0, flags);
    if(data) {
        uint32 formatSize = GetFormatSize(textureDesc.Format);

        D3D12_SUBRESOURCE_DATA subresources[6];
        for(uint32 i = 0; i < 6; ++i) {
            auto& subresource = subresources[i];
            subresource.RowPitch = width * formatSize;
            subresource.SlicePitch = width * height * formatSize;
            subresource.pData = data;
        }

        return CreateTexture(textureDesc, subresources, 6, initialState, mipUAVs);
    }
    return CreateTexture(textureDesc, nullptr, 0, initialState, mipUAVs);
}

// complete Dxteture field -> Create descriptor
static void initializeDepthTexture(ref<DxTexture> result, uint32 width, uint32 height, DXGI_FORMAT format, uint32 arrayLength, D3D12_RESOURCE_STATES initialState, bool allocateDescriptors)
{
    result->numMipLevels = 1;
    result->requestedNumMipLevels = 1;
    result->format = format;
    result->width = width;
    result->height = height;
    result->depth = arrayLength;

    D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport;
    formatSupport.Format = format;
    CheckResult(dxContext.device->CheckFeatureSupport(
        D3D12_FEATURE_FORMAT_SUPPORT,
        &formatSupport,
        sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));

    result->supportSRV = false;
    result->supportDSV = allocateDescriptors && FormatSupportsRtv(formatSupport);
    result->supportUAV = false;
    result->supportSRV = allocateDescriptors && FormatSupportsSrv(formatSupport);


    result->initialState = initialState;

    if (allocateDescriptors)
    {
        assert(result->supportDSV);

        result->dsvAllocation = dxContext.dsvHeap.Allocate();
        result->defaultDSV = DxDsvDescriptorHandle(result->dsvAllocation.CpuAt(0)).Create2DTextureDSV(result);

        if (arrayLength == 1)
        {
            result->srvUavAllocation = dxContext.srvUavHeap.Allocate(1 + isStencilFormat(format));
            result->defaultSRV = DxCpuDescriptorHandle(result->srvUavAllocation.CpuAt(0)).CreateDepthTextureSRV(result);

            if (isStencilFormat(format))
            {
                result->stencilSRV = DxCpuDescriptorHandle(result->srvUavAllocation.CpuAt(1)).CreateStencilTextureSRV(result);
            }
        }
        else
        {
            result->srvUavAllocation = dxContext.srvUavHeap.Allocate();
            result->defaultSRV = DxCpuDescriptorHandle(result->srvUavAllocation.CpuAt(0)).CreateDepthTextureArraySRV(result);
        }
    }
}

ref<DxTexture> CreateDepthTexture(uint32 width, uint32 height, DXGI_FORMAT format, uint32 arrayLength,
                                  D3D12_RESOURCE_STATES initialState)
{
    ref<DxTexture> result = makeRef<DxTexture>();
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = format;
    clearValue.DepthStencil = {1, 0};
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(getTypelessFormat(format), width, height, arrayLength, 1, 1, 0,
                                                            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    auto heapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    CheckResult(dxContext.device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &desc, initialState, &clearValue,
                                                          IID_PPV_ARGS(&result->resource)));
    initializeDepthTexture(result, width, height, format, arrayLength, initialState, true);
    return result;
    
}



static std::unordered_map<fs::path, weakRef<DxTexture>> textureCache; // TODO: Pack flags into key.
static std::mutex mutex;
ref<DxTexture> LoadTextureFromFile(const fs::path& filename, uint32 flags)
{
    mutex.lock();
    ref<DxTexture> sp = textureCache[filename].lock();
    if(!sp) {
        sp = LoadTextureInternal(filename, flags);
        textureCache[filename] = sp;
    }
    mutex.unlock();
    return sp;
}


static ref<DxTexture> loadTextureFromMemoryInternal(const void* ptr, uint32 size, image_format imageFormat,
                                                    const fs::path& cachePath, uint32 flags)
{
    DirectX::ScratchImage scratchImage;
    D3D12_RESOURCE_DESC textureDesc;

    if(!loadImageFromMemory(ptr, size, imageFormat, cachePath, flags, scratchImage, textureDesc)) {
        return nullptr;
    }

    return UploadImageToGpu(scratchImage, textureDesc, flags);
}

//used by assimp load
ref<DxTexture> LoadTextureFromMemory(const void* ptr, uint32 size, image_format format, const fs::path& cachePath, uint32 flags)
{
    mutex.lock();
    ref<DxTexture> sp = textureCache[cachePath].lock();
    if(!sp) {
        sp = loadTextureFromMemoryInternal(ptr, size, format, cachePath, flags);
        textureCache[cachePath] = sp;
        mutex.unlock();
        return sp;
    }
}