// ReSharper disable CppInconsistentNaming
#pragma once
#include <d3d12.h>

#include "Dx.h"
#include "DxDescriptorAllocation.h"


struct DxDynamicConstantBuffer
{
    D3D12_GPU_VIRTUAL_ADDRESS gpuPtr;
};

struct DxDynamicVertexBuffer
{
    D3D12_VERTEX_BUFFER_VIEW view;
};

struct DxDynamicIndexBuffer
{
    D3D12_INDEX_BUFFER_VIEW view;
};

struct DxBuffer
{
    DxBuffer(const DxBuffer&) = delete;
    DxBuffer(DxBuffer&&) = default;
    DxBuffer() = default;
    virtual ~DxBuffer();

    Dx12Resource resource;
    //Represents single memory allocation.
    D3D12MA::Allocation* allocation = nullptr;

    //Unlike texture, buffer use srvUav resource only.called by dxContext.srvUavHeap
    DxDescriptorAllocation descriptorAllocation;
    //used for supportClearing for now 
    DxDescriptorAllocation descriptorAllocationShaderVisible;

    DxCpuDescriptorHandle defaultSRV{};
    DxCpuDescriptorHandle defaultUAV{};

    //used only in culling of tileBasedRendering Pipeline for now
    DxCpuDescriptorHandle cpuClearUAV{};
    DxGpuDescriptorHandle gpuClearUAV{};

    DxCpuDescriptorHandle rayTracingSRV{};
    D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress{};

    bool supportUAV{};
    bool supportSRV{};
    bool supportClearing{};
    bool supportRayTracing{};

    uint32 elementSize = 0;
    uint32 elementCount = 0;
    uint32 totalSize = 0;
    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
};

struct DxVertexBuffer : DxBuffer
{
    D3D12_VERTEX_BUFFER_VIEW view;
};

struct DxIndexBuffer : DxBuffer
{
    D3D12_INDEX_BUFFER_VIEW view;
};

struct DxVertexBufferView
{
    D3D12_VERTEX_BUFFER_VIEW view{};
    DxVertexBufferView() { view.SizeInBytes = 0; }
    DxVertexBufferView(const ref<DxVertexBuffer>& vb)
    {
        view = vb ? vb->view : D3D12_VERTEX_BUFFER_VIEW{0};
    }
    DxVertexBufferView(const DxDynamicVertexBuffer& vb)
        : view(vb.view)
    {
    }
    DxVertexBufferView(const DxVertexBufferView& other) = default;
    DxVertexBufferView(DxVertexBufferView&& other) noexcept = default;
    DxVertexBufferView& operator=(const DxVertexBufferView& other) = default;
    DxVertexBufferView& operator=(DxVertexBufferView&& other) noexcept = default;

    operator bool() const { return view.SizeInBytes > 0; }
    operator const D3D12_VERTEX_BUFFER_VIEW&() const { return view; }
};

struct DxIndexBufferView
{
    D3D12_INDEX_BUFFER_VIEW view;
    DxIndexBufferView() { view.SizeInBytes = 0; }
    DxIndexBufferView(const ref<DxIndexBuffer>& ib)
        : view(ib->view)
    {
    }
    DxIndexBufferView(const DxDynamicIndexBuffer& ib)
        : view(ib.view)
    {
    }

    DxIndexBufferView(const DxIndexBufferView& other) = default;
    DxIndexBufferView(DxIndexBufferView&& other) noexcept = default;
    DxIndexBufferView& operator=(const DxIndexBufferView& other) = default;
    DxIndexBufferView& operator=(DxIndexBufferView&& other) noexcept = default;

    operator bool() const { return view.SizeInBytes > 0; }
    operator const D3D12_INDEX_BUFFER_VIEW&() const { return view; }
};

struct VertexBufferGroup
{
    ref<DxVertexBuffer> positions;
    ref<DxVertexBuffer> others; //UV, normals, tangents, etc
};

//srv & uav
struct BufferGrave
{
    Dx12Resource resource;
    DxDescriptorAllocation descriptorAllocation = {};
    DxDescriptorAllocation descriptorAllocationShaderVisible = {};

    BufferGrave() = default;

    BufferGrave(const BufferGrave& other) = delete;
    BufferGrave(BufferGrave&& other) noexcept
    {
    }
    BufferGrave& operator=(const BufferGrave& other) = delete;
    BufferGrave& operator=(BufferGrave&& other) noexcept
    {
        if(this == &other)
            return *this;
        return *this;
    }
    ~BufferGrave();
};

struct DxMesh
{
    VertexBufferGroup vertexBuffer;
    ref<DxIndexBuffer> indexBuffer;
};

struct MapRange
{
    uint32 firstElement = 0;
    uint32 numElements = static_cast<uint32>(-1);
};

//should only map upload and readBack buffer.Upload buffer is used for writing and readBack Buffer is used only for read;
void* MapBuffer(const ref<DxBuffer>& buffer, bool intentsReading, MapRange readRange = {});
void UnMapBuffer(const ref<DxBuffer>& buffer, bool hasWritten, MapRange writeRange = {});

ref<DxBuffer> CreateBuffer(uint32 elementSize, uint32 elementCount, void* data, bool allowUnorderedAccess = false,
                           bool allowClearing = false, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON);
ref<DxBuffer> CreateReadBackBuffer(uint32 elementSize, uint32 elementCount,
                                   D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST);
ref<DxBuffer> CreateUploadBuffer(uint32 elementSize, uint32 elementCount, void* data);

void ResizeBuffer(ref<DxBuffer> buffer, uint32 newElementCount, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON);