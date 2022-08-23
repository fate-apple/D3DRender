#include "pch.h"
#include "DxDescriptor.h"

#include "DxBuffer.h"
#include "DxContext.h"
#include "core/Image.h"

// The D3D12_SHADER_COMPONENT_MAPPING enumeration specifies what values from memory should be returned when the texture is accessed in a shader via this shader resource view (SRV).
// This enum allows the SRV to select how memory gets routed to the four return components in a shader after a memory fetch.
// The options for each shader component [0..3] (corresponding to RGBA) are: component 0..3 from the SRV fetch result or force 0 or force 1.
static uint32 GetShader4ComponentMapping(DXGI_FORMAT format)
{
    switch(getNumberOfChannels(format)) {
    case 1:
        return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
            D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
    case 2:
        return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
            D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
            D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
    case 3:
        return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
            D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
    case 4:
        return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3);
    default:
        std::cerr << "GetNumberOfChannels(format)";
    }
    return D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::CreateBufferSRV(const ref<DxBuffer>& buffer, BufferRange bufferRange)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = bufferRange.firstElement;
    srvDesc.Buffer.NumElements = bufferRange.numElements;
    srvDesc.Buffer.StructureByteStride = 0;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

    dxContext.device->CreateShaderResourceView(buffer->resource.Get(), &srvDesc, cpuHandle);
    return *this;
}

/**
 * \brief view element as 32 bit
 * \param buffer 
 * \param bufferRange 
 * \return 
 */
DxCpuDescriptorHandle& DxCpuDescriptorHandle::CreateRawBufferSRV(const ref<DxBuffer>& buffer, BufferRange bufferRange)
{
    uint32 firstElementByteOffset = bufferRange.firstElement * buffer->elementSize;
    assert(firstElementByteOffset % 16 == 0);

    uint32 count = (bufferRange.numElements != -1) ? bufferRange.numElements : (buffer->elementCount - bufferRange.firstElement);
    uint32 totalSize = count * buffer->elementSize;
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = firstElementByteOffset / 4;
    srvDesc.Buffer.NumElements = totalSize / 4;
    srvDesc.Buffer.StructureByteStride = buffer->elementSize;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    dxContext.device->CreateShaderResourceView(buffer->resource.Get(), &srvDesc, cpuHandle);
    return *this;
}


DxCpuDescriptorHandle& DxCpuDescriptorHandle::CreateBufferUAV(const ref<DxBuffer>& buffer, BufferRange bufferRange)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    //uavDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    //uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    //If pCounterResource is not specified, then CounterOffsetInBytes must be 0
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    uavDesc.Buffer.FirstElement = bufferRange.firstElement;
    uavDesc.Buffer.NumElements = bufferRange.numElements == static_cast<decltype(bufferRange.numElements)>(-1)
                                     ? (buffer->elementCount - bufferRange.firstElement)
                                     : bufferRange.numElements;
    uavDesc.Buffer.StructureByteStride = buffer->elementSize;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    dxContext.device->CreateUnorderedAccessView(buffer->resource.Get(), nullptr, &uavDesc, cpuHandle);
    return *this;
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::CreateBufferUintUAV(const ref<DxBuffer>& buffer, BufferRange bufferRange)
{
    uint32 firstElementByteOffset = bufferRange.firstElement * buffer->elementSize;
    assert(firstElementByteOffset % 16 == 0);
    uint32 count = bufferRange.numElements == static_cast<decltype(bufferRange.numElements)>(-1)
                       ? (buffer->elementCount - bufferRange.firstElement)
                       : bufferRange.numElements;
    uint32 totalSize = count * buffer->elementSize;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    //If pCounterResource is not specified, then CounterOffsetInBytes must be 0
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    uavDesc.Buffer.FirstElement = firstElementByteOffset / 4;
    uavDesc.Buffer.NumElements = totalSize / 4;
    //fi the RAW flag is not set and StructureByteStride = 0, then the format must be a valid UAV format.
    uavDesc.Buffer.StructureByteStride = 0;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    dxContext.device->CreateUnorderedAccessView(buffer->resource.Get(), nullptr, &uavDesc, cpuHandle);
    return *this;
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::CreateRaytracingAccelerationStructureSRV(const ref<DxBuffer>& tlas)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = tlas->gpuVirtualAddress;

    dxContext.device->CreateShaderResourceView(nullptr, &srvDesc, cpuHandle);
    return *this;
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::Create2DTextureSRV(const ref<DxTexture>& texture, TextureMipRange mipRange,
                                                                 DXGI_FORMAT format)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Format = (format == DXGI_FORMAT_UNKNOWN) ? texture->resource->GetDesc().Format : format;
    desc.Shader4ComponentMapping = GetShader4ComponentMapping(desc.Format);
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MostDetailedMip = mipRange.first; //0 default
    desc.Texture2D.MipLevels = mipRange.count; //-1 default
    dxContext.device->CreateShaderResourceView(texture->resource.Get(), &desc, cpuHandle);
    return *this;
}


DxCpuDescriptorHandle& DxCpuDescriptorHandle::CreateNullTextureSRV()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 0;
    
    //At least one of pResource or pDesc must be provided. A null pResource is used to initialize a null descriptor, which guarantees D3D11-like null binding behavior (reading 0s, writes are discarded),
    //but must have a valid pDesc in order to determine the descriptor type.
    //Describes the CPU descriptor handle that represents the shader-resource view. This handle can be created in a shader-visible or non-shader-visible descriptor heap.
    dxContext.device->CreateShaderResourceView(nullptr, &srvDesc, cpuHandle);
    return *this;
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::Create2DTextureUAV(const ref<DxTexture>& texture, uint32 mipSlice,
                                                                 DXGI_FORMAT overrideFormat)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.Format = overrideFormat;
    desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    //The mipmap slice index.
    desc.Texture2D.MipSlice = mipSlice;
    dxContext.device->CreateUnorderedAccessView(texture->resource.Get(), nullptr, &desc, cpuHandle);
    return *this;
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::CreateCubemapSRV(const ref<DxTexture>& texture, TextureMipRange mipRange,
                                                               DXGI_FORMAT format)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
    desc.Format = (format == DXGI_FORMAT_UNKNOWN) ? texture->resource->GetDesc().Format : format;
    desc.Shader4ComponentMapping = GetShader4ComponentMapping(desc.Format);
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    desc.TextureCube.MostDetailedMip = mipRange.first;
    desc.TextureCube.MipLevels = mipRange.count;
    dxContext.device->CreateShaderResourceView(texture->resource.Get(), &desc, cpuHandle);
    return *this;
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::CreateCubemapUAV(const ref<DxTexture>& texture, uint32 mipSlice,
                                                               DXGI_FORMAT overrideFormat)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
    desc.Format = overrideFormat;
    desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY; //different to srv; write TEXTURE2DARRAY of depth-6 as cubeTexture
    desc.Texture2DArray.MipSlice = mipSlice;
    desc.Texture2DArray.FirstArraySlice = 0;
    desc.Texture2DArray.ArraySize = texture->resource->GetDesc().DepthOrArraySize;
    dxContext.device->CreateUnorderedAccessView(texture->resource.Get(), nullptr, &desc, cpuHandle);
    return *this;
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::CreateVolumeTextureSRV(const ref<DxTexture>& texture, TextureMipRange mipRange,
                                                                     DXGI_FORMAT format)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
    desc.Format = (format == DXGI_FORMAT_UNKNOWN) ? texture->resource->GetDesc().Format : format;
    desc.Shader4ComponentMapping = GetShader4ComponentMapping(desc.Format);
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    desc.Texture3D.MostDetailedMip = mipRange.first;
    desc.Texture3D.MipLevels = mipRange.count;
    desc.Texture3D.ResourceMinLODClamp = 0;
    dxContext.device->CreateShaderResourceView(texture->resource.Get(), &desc, cpuHandle);
    return *this;
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::CreateVolumeTextureUAV(const ref<DxTexture>& texture, uint32 mipSlice,
                                                                     DXGI_FORMAT overrideFormat)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
    desc.Format = overrideFormat;
    desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D; //unlike 2dArray, 3d can use float and do filter in w. 
    desc.Texture3D.MipSlice = mipSlice;
    desc.Texture3D.FirstWSlice = 0;
    desc.Texture3D.WSize = texture->depth;
    dxContext.device->CreateUnorderedAccessView(texture->resource.Get(), nullptr, &desc, cpuHandle);
    return *this;
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::CreateDepthTextureSRV(const ref<DxTexture>& texture)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc;
    desc.Format = GetDepthReadFormat(texture->format);
    //an arbitrary mapping can be specified using the macro D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING.
    desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 0, 0, 0);
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipLevels = 1;
    dxContext.device->CreateShaderResourceView(texture->resource.Get(), &desc, cpuHandle);
    return *this;
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::CreateStencilTextureSRV(const ref<DxTexture>& texture)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc;
    desc.Format = GetStencilReadFormat(texture->format);
    //The default 1:1 mapping can be indicated by specifying D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipLevels = 1;
    //The index (plane slice number) of the plane to use in the texture. 
    desc.Texture2D.PlaneSlice = 1;
    //CalcSubresource index =  MipSlice + ArraySlice * MipLevels + PlaneSlice * MipLevels * ArraySize
    dxContext.device->CreateShaderResourceView(texture->resource.Get(), &desc, cpuHandle);
    return *this;
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::CreateDepthTextureArraySRV(const ref<DxTexture>& texture)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc;
    desc.Format = GetDepthReadFormat(texture->format);
    //an arbitrary mapping can be specified using the macro D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING.
    desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 0, 0, 0);
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    desc.Texture2DArray.MipLevels = 1;
    desc.Texture2DArray.FirstArraySlice = 0;
    desc.Texture2DArray.ArraySize = texture->resource->GetDesc().DepthOrArraySize;
    dxContext.device->CreateShaderResourceView(texture->resource.Get(), &desc, cpuHandle);
    return *this;
}

DxCpuDescriptorHandle DxCpuDescriptorHandle::operator+(uint32 i)
{
    return {
        CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuHandle, static_cast<INT>(i), dxContext.descriptorHandleIncrementSizeCBV_SRV_UAV)
    };
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::operator+=(uint32 i)
{
    cpuHandle.Offset(i, dxContext.descriptorHandleIncrementSizeCBV_SRV_UAV);
    return *this;
}

DxCpuDescriptorHandle& DxCpuDescriptorHandle::operator++()
{
    cpuHandle.Offset(1, dxContext.descriptorHandleIncrementSizeCBV_SRV_UAV);
    return *this;
}

DxCpuDescriptorHandle DxCpuDescriptorHandle::operator++(int)
{
    const DxCpuDescriptorHandle result = *this;
    cpuHandle.Offset(1, dxContext.descriptorHandleIncrementSizeCBV_SRV_UAV);
    return result;
}

DxGpuDescriptorHandle DxGpuDescriptorHandle::operator+(uint32 i)
{
    return {
        CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuHandle, static_cast<INT>(i), dxContext.descriptorHandleIncrementSizeCBV_SRV_UAV)
    };
}

DxGpuDescriptorHandle& DxGpuDescriptorHandle::operator+=(uint32 i)
{
    gpuHandle.Offset(i, dxContext.descriptorHandleIncrementSizeCBV_SRV_UAV);
    return *this;
}

DxGpuDescriptorHandle& DxGpuDescriptorHandle::operator++()
{
    gpuHandle.Offset(1, dxContext.descriptorHandleIncrementSizeCBV_SRV_UAV);
    return *this;
}

DxGpuDescriptorHandle DxGpuDescriptorHandle::operator++(int)
{
    const DxGpuDescriptorHandle result = *this;
    gpuHandle.Offset(1, dxContext.descriptorHandleIncrementSizeCBV_SRV_UAV);
    return result;
}



DxRtvDescriptorHandle& DxRtvDescriptorHandle::Create2DTextureRTV(const ref<DxTexture>& texture, uint32 arraySlice,
                                                                 uint32 mipSlice)
{
    assert(texture->supportRTV);
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
    rtvDesc.Format = texture->format;
    if(texture->depth == 1) {
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        //A mip slice includes one mipmap level for every texture in an array. Texture2D.MipSlice field is  The index of the mipmap level to use.
    } else {
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.FirstArraySlice = arraySlice;
        rtvDesc.Texture2DArray.ArraySize = 1;
    }
    rtvDesc.Texture2D.MipSlice = mipSlice;
    rtvDesc.Texture2D.PlaneSlice = 0;
    //Creates a render-target view for accessing resource data.#
    dxContext.device->CreateRenderTargetView(texture->resource.Get(), &rtvDesc, cpuHandle);
    return *this;
}

DxRtvDescriptorHandle& DxRtvDescriptorHandle::CreateNullTextureRTV(DXGI_FORMAT format)
{
    D3D12_RENDER_TARGET_VIEW_DESC desc{};
    desc.Format = format;
    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    dxContext.device->CreateRenderTargetView(0, &desc, cpuHandle);
    return *this;
}

DxDsvDescriptorHandle& DxDsvDescriptorHandle::Create2DTextureDSV(const ref<DxTexture>& texture, uint32 arraySlice,
                                                                 uint32 mipSlice)
{
    assert(texture->supportDSV);

    D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
    desc.Format = texture->format;
    if(texture->depth == 1) {
        desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = mipSlice;
    } else {
        desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        desc.Texture2DArray.FirstArraySlice = arraySlice;
        desc.Texture2DArray.ArraySize = 1;
        desc.Texture2DArray.MipSlice = mipSlice;
    }
    dxContext.device->CreateDepthStencilView(texture->resource.Get(), &desc, cpuHandle);
    return *this;
}