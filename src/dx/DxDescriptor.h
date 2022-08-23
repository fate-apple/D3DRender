#pragma once
#include "Dx.h"
//#include "DxBuffer.h"
//#include "DxTexture.h"

//forward define
struct DxTexture;
struct DxBuffer;

struct TextureMipRange
{
    uint32 first = 0;
    uint32 count = static_cast<uint32>(-1);
};
struct BufferRange
{
    uint32 firstElement = 0;
    uint32 numElements = static_cast<uint32>(-1);
};

//srv & uav
struct DxCpuDescriptorHandle
{
    DxCpuDescriptorHandle() = default;
    DxCpuDescriptorHandle(CD3DX12_CPU_DESCRIPTOR_HANDLE handle)
        : cpuHandle(handle)
    {
    }
    DxCpuDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
        : cpuHandle(handle)
    {
    }
    explicit DxCpuDescriptorHandle(CD3DX12_DEFAULT)
        : cpuHandle(D3D12_DEFAULT)
    {
    }


    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    inline operator CD3DX12_CPU_DESCRIPTOR_HANDLE() const { return cpuHandle; }
    inline operator bool() const { return cpuHandle.ptr != 0; }

    DxCpuDescriptorHandle& CreateBufferSRV(const ref<DxBuffer>& buffer, BufferRange bufferRange = {});
    DxCpuDescriptorHandle& CreateRawBufferSRV(const ref<DxBuffer>& buffer, BufferRange range= {});
    DxCpuDescriptorHandle& CreateBufferUAV(const ref<DxBuffer>& buffer, BufferRange bufferRange = {});
    DxCpuDescriptorHandle& CreateBufferUintUAV(const ref<DxBuffer>& buffer, BufferRange bufferRange = {});
    //TODO: sjw. learn more about this
    DxCpuDescriptorHandle& CreateRaytracingAccelerationStructureSRV(const ref<DxBuffer>& tlas);

    DxCpuDescriptorHandle& Create2DTextureSRV(const ref<DxTexture>& texture, TextureMipRange mipRange = {},
                                              DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
    DxCpuDescriptorHandle& CreateNullTextureSRV();
    DxCpuDescriptorHandle& Create2DTextureUAV(const ref<DxTexture>& texture, uint32 mipSlice = 0,
                                              DXGI_FORMAT overrideFormat = DXGI_FORMAT_UNKNOWN);
    DxCpuDescriptorHandle& CreateCubemapSRV(const ref<DxTexture>& texture, TextureMipRange mipRange = {},
                                            DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
    DxCpuDescriptorHandle& CreateCubemapUAV(const ref<DxTexture>& texture, uint32 mipSlice = 0,
                                            DXGI_FORMAT overrideFormat = DXGI_FORMAT_UNKNOWN);
    DxCpuDescriptorHandle& CreateVolumeTextureSRV(const ref<DxTexture>& texture, TextureMipRange mipRange = {},
                                                  DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
    DxCpuDescriptorHandle& CreateVolumeTextureUAV(const ref<DxTexture>& texture, uint32 mipSlice = 0,
                                                  DXGI_FORMAT overrideFormat = DXGI_FORMAT_UNKNOWN);
    DxCpuDescriptorHandle& CreateDepthTextureSRV(const ref<DxTexture>& texture);
    DxCpuDescriptorHandle& CreateDepthTextureArraySRV(const ref<DxTexture>& texture);
    DxCpuDescriptorHandle& CreateStencilTextureSRV(const ref<DxTexture>& texture);
    

    DxCpuDescriptorHandle
    operator+(uint32 i);
    DxCpuDescriptorHandle& operator+=(uint32 i);
    DxCpuDescriptorHandle& operator++();
    DxCpuDescriptorHandle operator++(int); // postfix ++, one copy cost
};

struct DxGpuDescriptorHandle
{
    DxGpuDescriptorHandle() = default;
    DxGpuDescriptorHandle(CD3DX12_GPU_DESCRIPTOR_HANDLE handle)
        : gpuHandle(handle)
    {
    }
    DxGpuDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle)
        : gpuHandle(handle)
    {
    }
    explicit DxGpuDescriptorHandle(CD3DX12_DEFAULT)
        : gpuHandle(D3D12_DEFAULT)
    {
    }
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
    inline operator D3D12_GPU_DESCRIPTOR_HANDLE() const { return gpuHandle; }
    inline operator bool() const { return gpuHandle.ptr != 0; }

    DxGpuDescriptorHandle operator+(uint32 i);
    DxGpuDescriptorHandle& operator+=(uint32 i);
    DxGpuDescriptorHandle& operator++();
    DxGpuDescriptorHandle operator++(int); // postfix ++, one copy cost
};

struct DxBothDescriptorHandle : DxCpuDescriptorHandle, DxGpuDescriptorHandle
{
};

struct DxRtvDescriptorHandle
{
    DxRtvDescriptorHandle() = default;
    DxRtvDescriptorHandle(CD3DX12_CPU_DESCRIPTOR_HANDLE handle)
        : cpuHandle(handle)
    {
    }
    DxRtvDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
        : cpuHandle(handle)
    {
    }
    DxRtvDescriptorHandle(CD3DX12_DEFAULT _)
        : cpuHandle(D3D12_DEFAULT)
    {
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;

    inline operator CD3DX12_CPU_DESCRIPTOR_HANDLE() const { return cpuHandle; }
    inline operator bool() const { return cpuHandle.ptr != 0; }

    DxRtvDescriptorHandle& Create2DTextureRTV(const ref<DxTexture>& texture, uint32 arraySlice = 0, uint32 mipSlice = 0);
    DxRtvDescriptorHandle& CreateNullTextureRTV(DXGI_FORMAT format);
};

//Do not add any other field. keep same with DxCpuDescriptorHandle in memory; we may use reinterpret_cast
struct DxDsvDescriptorHandle
{
    DxDsvDescriptorHandle() = default;
    DxDsvDescriptorHandle(CD3DX12_CPU_DESCRIPTOR_HANDLE handle)
        : cpuHandle(handle)
    {
    }
    DxDsvDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
        : cpuHandle(handle)
    {
    }
    DxDsvDescriptorHandle(CD3DX12_DEFAULT _)
        : cpuHandle(D3D12_DEFAULT)
    {
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;

    inline operator CD3DX12_CPU_DESCRIPTOR_HANDLE() const { return cpuHandle; }
    inline operator bool() const { return cpuHandle.ptr != 0; }

    DxDsvDescriptorHandle& Create2DTextureDSV(const ref<DxTexture>& texture, uint32 arraySlice = 0, uint32 mipSlice = 0);
};