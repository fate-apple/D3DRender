#include "pch.h"
#include "DxBuffer.h"

#include "DxContext.h"

static void Retire(Dx12Resource resource, DxDescriptorAllocation descriptorAllocation,
                   DxDescriptorAllocation descriptorAllocationShaderVisible)
{
    BufferGrave bufferGrave;
    //TODO: sjw. use move here?
    bufferGrave.resource = resource;
    bufferGrave.descriptorAllocation = descriptorAllocation;
    bufferGrave.descriptorAllocationShaderVisible = descriptorAllocationShaderVisible;
    dxContext.Retire(std::move(bufferGrave));
}

static void UploadBufferData(const ref<DxBuffer>& buffer, const void* bufferData)
{
    DxCommandList* cl = dxContext.GetFreeCopyCommandList();
    Dx12Resource intermediateResource;
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(buffer->totalSize);

    CD3DX12_HEAP_PROPERTIES properties(D3D12_HEAP_TYPE_UPLOAD);
    CheckResult(dxContext.device->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&intermediateResource)));

    // ReSharper disable once CppInitializedValueIsAlwaysRewritten
    D3D12_SUBRESOURCE_DATA subresourceData{};
    subresourceData.pData = bufferData;
    subresourceData.RowPitch = buffer->totalSize;
    //The row pitch, or width, or physical size, in bytes, of the subresource data.
    subresourceData.SlicePitch = subresourceData.RowPitch;
    //The depth pitch, or width, or physical size, in bytes, of the subresource data.

    cl->TransitionBarrier(buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    // second parameter and third parameter represents the destination resource and intermediate resource respectively
    UpdateSubresources(cl->commandList.Get(),
                       buffer->resource.Get(), intermediateResource.Get(),
                       0, 0, 1, &subresourceData);

    // We are omit the transition, since the resource automatically decays to common state after being accessed on a copy queue.
    cl->TransitionBarrier(buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
    dxContext.Retire(intermediateResource);
    dxContext.ExecuteCommandList(cl);
}

void* MapBuffer(const ref<DxBuffer>& buffer, bool intentsReading, MapRange readRange)
{
    D3D12_RANGE range = {0, 0};
    D3D12_RANGE* rangePtr = nullptr;

    if(intentsReading) {
        if(readRange.numElements != static_cast<decltype(readRange.numElements)>(-1)) {
            range.Begin = readRange.firstElement * static_cast<SIZE_T>(buffer->elementSize);
            range.End = range.Begin + readRange.numElements * static_cast<SIZE_T>(buffer->elementSize);
            rangePtr = &range;
        }
    } else {
        rangePtr = &range;
    }

    void* result;
    //A null pointer indicates the entire subresource might be read by the CPU.
    //It is valid to specify the CPU won't read any data by passing a range where End is less than or equal to Begin.
    buffer->resource->Map(0, rangePtr, &result);
    return result;
}

void UnMapBuffer(const ref<DxBuffer>& buffer, bool hasWritten, MapRange writeRange)
{
    D3D12_RANGE range = {0, 0};
    D3D12_RANGE* rangePtr = nullptr;

    if(hasWritten) {
        if(writeRange.numElements != static_cast<decltype(writeRange.numElements)>(-1)) {
            const auto size = static_cast<decltype(range.Begin)>(buffer->elementSize);
            range.Begin = writeRange.firstElement * size;
            range.End = range.Begin + writeRange.numElements * size;
            rangePtr = &range;
        }
    } else {
        rangePtr = &range;
    }
    buffer->resource->Unmap(0, rangePtr);
}


// set struct field -> Create Committed Resource (desc use CD3DX12_RESOURCE_DESC::Buffer) -> upload data when needed
static void InitializeBuffer(ref<DxBuffer> buffer, uint32 elementSize, uint32 eleCount,
                             void* data, bool allowUnorderedAccess, bool allowClear, bool inRaytracing = false,
                             D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON,
                             D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT)
{
    D3D12_RESOURCE_FLAGS flags = allowUnorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
    buffer->elementCount = eleCount;
    buffer->elementSize = elementSize;
    buffer->totalSize = eleCount * elementSize;
    buffer->heapType = heapType;
    //This heap type is best for GPU-write-once, CPU-readable data.
    buffer->supportSRV = (heapType != D3D12_HEAP_TYPE_READBACK);
    buffer->supportUAV = allowUnorderedAccess;
    buffer->supportClearing = allowClear;
    buffer->supportRayTracing = inRaytracing;

    buffer->defaultSRV = {};
    buffer->defaultUAV = {};
    buffer->cpuClearUAV = {};
    buffer->gpuClearUAV = {};
    buffer->rayTracingSRV = {};

    auto desc = CD3DX12_RESOURCE_DESC::Buffer(buffer->totalSize, flags);
    CD3DX12_HEAP_PROPERTIES properties(heapType);
    CheckResult(dxContext.device->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        initialState,
        0,
        IID_PPV_ARGS(&buffer->resource)));

    buffer->gpuVirtualAddress = buffer->resource->GetGPUVirtualAddress();

    //Upload
    if(data) {
        if(heapType == D3D12_HEAP_TYPE_DEFAULT) {
            UploadBufferData(buffer, data);
        } else if(heapType == D3D12_HEAP_TYPE_UPLOAD) {
            void* dataPtr = MapBuffer(buffer, false); //specify the CPU won't read any data 
            memcpy(dataPtr, data, buffer->totalSize);
            UnMapBuffer(buffer, true); //indicates the entire subresource might have been modified by the CPU.
        }
    }
    const uint32 numDescriptors = buffer->supportSRV + buffer->supportUAV + buffer->supportClearing + buffer->supportRayTracing;
    buffer->descriptorAllocation = dxContext.srvUavHeap.Allocate(numDescriptors);
    uint32 index(0);
    if(buffer->supportSRV) {
        buffer->defaultSRV = DxCpuDescriptorHandle(buffer->descriptorAllocation.CpuAt(index)).CreateBufferSRV(buffer);
        index++;
    }
    if(buffer->supportUAV) {
        buffer->defaultUAV = DxCpuDescriptorHandle(buffer->descriptorAllocation.CpuAt(index)).CreateBufferUAV(buffer);
        index++;
    }
    if(buffer->supportClearing) {
        buffer->cpuClearUAV = DxCpuDescriptorHandle(buffer->descriptorAllocation.CpuAt(index)).CreateBufferUintUAV(buffer);
        index++;
        buffer->descriptorAllocationShaderVisible = dxContext.srvUavHeapShaderVisible.Allocate(1);
        buffer->gpuClearUAV = DxGpuDescriptorHandle(buffer->descriptorAllocationShaderVisible.GpuAt(0));
    }
    if(buffer->supportRayTracing) {
        buffer->rayTracingSRV = DxCpuDescriptorHandle(buffer->descriptorAllocation.CpuAt(index)).
        CreateRaytracingAccelerationStructureSRV(buffer);
        index++;
    }
    assert(index == numDescriptors);
}


ref<DxBuffer> CreateBuffer(uint32 elementSize, uint32 elementCount, void* data, bool allowUnorderedAccess, bool allowClearing,
                           D3D12_RESOURCE_STATES initialState)
{
    ref<DxBuffer> result = makeRef<DxBuffer>();
    InitializeBuffer(result, elementSize, elementCount, data, allowUnorderedAccess, allowClearing, false, initialState,
                     D3D12_HEAP_TYPE_DEFAULT);
    return result;
}

ref<DxBuffer> CreateReadBackBuffer(uint32 elementSize, uint32 elementCount, D3D12_RESOURCE_STATES initialState)
{
    ref<DxBuffer> result = makeRef<DxBuffer>();
    InitializeBuffer(result, elementSize, elementCount, nullptr, false, false, false, initialState, D3D12_HEAP_TYPE_READBACK);
    return result;
}

ref<DxBuffer> CreateUploadBuffer(uint32 elementSize, uint32 elementCount, void* data)
{
    ref<DxBuffer> result = makeRef<DxBuffer>();
    InitializeBuffer(result, elementSize, elementCount, data, false, false, false, D3D12_RESOURCE_STATE_GENERIC_READ,
                     D3D12_HEAP_TYPE_UPLOAD);
    return result;
}

void ResizeBuffer(ref<DxBuffer> buffer, uint32 newElementCount, D3D12_RESOURCE_STATES initialState)
{
    Retire(buffer->resource, buffer->descriptorAllocation, buffer->descriptorAllocationShaderVisible);
    if(buffer->allocation) dxContext.Retire(buffer->allocation);
    buffer->elementCount = newElementCount;
    buffer->totalSize = buffer->elementCount * buffer->elementSize;;
    D3D12_RESOURCE_DESC desc = buffer->resource->GetDesc();
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(buffer->totalSize, desc.Flags);

    CD3DX12_HEAP_PROPERTIES properties(buffer->heapType);
    CheckResult(dxContext.device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &bufferDesc, initialState, nullptr,
                                                          IID_PPV_ARGS(&buffer->resource)));
    buffer->gpuVirtualAddress = buffer->resource->GetGPUVirtualAddress();
    uint32 numDescriptors = buffer->supportSRV + buffer->supportUAV + buffer->supportClearing + buffer->supportRayTracing;
    buffer->descriptorAllocation = dxContext.srvUavHeap.Allocate(numDescriptors);
    uint32 index = 0;
    if(buffer->supportSRV) {
        buffer->defaultSRV = DxCpuDescriptorHandle(buffer->descriptorAllocation.CpuAt(index++)).CreateBufferSRV(buffer);
    }
    if(buffer->supportUAV) {
        buffer->defaultUAV = DxCpuDescriptorHandle(buffer->descriptorAllocation.CpuAt(index++)).CreateBufferUAV(buffer);
    }
    if(buffer->supportClearing) {
        buffer->cpuClearUAV = DxCpuDescriptorHandle(buffer->descriptorAllocation.CpuAt(index++)).CreateBufferUintUAV(buffer);
        buffer->descriptorAllocationShaderVisible = dxContext.srvUavHeapShaderVisible.Allocate(1);
        DxCpuDescriptorHandle(buffer->descriptorAllocationShaderVisible.CpuAt(0)).CreateBufferUintUAV(buffer);
        buffer->gpuClearUAV = buffer->descriptorAllocationShaderVisible.GpuAt(0);
    }
    if(buffer->supportRayTracing) {
        buffer->rayTracingSRV = DxCpuDescriptorHandle(buffer->descriptorAllocation.CpuAt(index++)).CreateRaytracingAccelerationStructureSRV(buffer);
    }
    assert(index == numDescriptors);
}


DxBuffer::~DxBuffer()
{
    Retire(resource, descriptorAllocation, descriptorAllocationShaderVisible);
    if(allocation) {
        dxContext.Retire(allocation);
    }
}

BufferGrave::~BufferGrave()
{
    if(resource) {
        dxContext.srvUavHeap.Free(descriptorAllocation);
        dxContext.srvUavHeapShaderVisible.Free(descriptorAllocationShaderVisible);
    }
}