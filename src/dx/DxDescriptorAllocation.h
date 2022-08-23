#pragma once
#include <mutex>

#include "Dx.h"
#include "DxDescriptor.h"
#include "core/BlockAllocator.h"


//minimum unit of once Descriptor Allocation, keep a range of descriptors in Descriptor Page's Dx12DescriptorHeap.
//note we can create multiple descriptor and store continuously in one Allocation
struct DxDescriptorAllocation
{
    uint64 count = 0;

    inline CD3DX12_CPU_DESCRIPTOR_HANDLE CpuAt(const uint32 index = 0)
    {
        assert(index < count);
        return {_cpuBase, static_cast<INT>(index), _descriptorSize};
    }
    inline CD3DX12_GPU_DESCRIPTOR_HANDLE GpuAt(const uint32 index = 0)
    {
        assert(index < count);
        return {_gpuBase, static_cast<INT>(index), _descriptorSize};
    }
    [[nodiscard]] inline bool IsValid() const { return count > 0; }
private:
    //Descriptor Handle in DxDescriptorPage.descriptorHeap
    CD3DX12_CPU_DESCRIPTOR_HANDLE _cpuBase = {};
    CD3DX12_GPU_DESCRIPTOR_HANDLE _gpuBase = {};
    uint32 _descriptorSize = 0;
    uint32 _pageIndex = 0;

    friend struct DxDescriptorHeap;
    friend struct DxDescriptorPage;
};

//keep Dx12DescriptorHeap where Descriptors stored in, impl actual Allocate & Free;
//hold a Dx12DescriptorHeap, which has same type to all pages in DxDescriptorHeap.
struct DxDescriptorPage
{
    D3D12_DESCRIPTOR_HEAP_TYPE type;
    Dx12DescriptorHeap descriptorHeap;

    //Descriptor Handle for heap start
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuBase;
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBase;
    uint32 descriptorSize;
    //note descriptor actually store in heap; blockAllocator just keep track whether descriptors used 
    //so blockAllocator.capacity = Dx12DescriptorHeap.desc.NumDescriptors, eg. num of descriptor stored in this DxDescriptorPage.descriptorHeap
    BlockAllocator blockAllocator;      

    DxDescriptorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint64 capacity, bool shaderVisible);
    std::pair<DxDescriptorAllocation, bool> Allocate(uint64 count);
    void Free(DxDescriptorAllocation allocation);
};

//Control Descriptor  all pages
struct DxDescriptorHeap
{
    DxDescriptorHeap() = default;
    explicit DxDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE type)
        : type(type)
    {
    }

    D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;

    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible, uint64 pageSize = 1024);
    DxDescriptorAllocation Allocate(uint64 count = 1);
    void Free(DxDescriptorAllocation allocation);
private:
    std::vector<DxDescriptorPage*> _pages;
    uint64 _pageSize = 0;
    bool _shaderVisible = false;
};



struct DxDescriptorRange
{
    Dx12DescriptorHeap descriptorHeap;
    D3D12_DESCRIPTOR_HEAP_TYPE type;
    uint32 descriptorHandleIncrementSize;
};

struct DxFrameDescriptorPage
{
    Dx12DescriptorHeap descriptorHeap;
    DxBothDescriptorHandle base;
    uint32 usedDescriptors;
    uint32 maxNumDescriptors;
    uint32 descriptorHandleIncrementSize;

    DxFrameDescriptorPage* next;
};

struct DxFrameDescriptorAllocator
{
    DxFrameDescriptorPage* usedPages[NUM_BUFFERED_FRAMES];
    DxFrameDescriptorPage* freePages;
    uint32 currentFrame;

    std::mutex mutex;

    void Initialize();
    void NewFrame(uint32 bufferedFrameID);
    DxDescriptorRange AllocateContiguousDescriptorRange(uint32 count);
};

/**
 * \brief CBV_SRV_UAV heap with maxSize; push equal to descriptor Offset(1)
 */
struct DxPushableDescriptorHeap
{
    Dx12DescriptorHeap descriptorHeap;
    DxCpuDescriptorHandle currentCpu;
    DxGpuDescriptorHandle currentGpu;
    
    void Initialize(uint32 maxSize, bool shaderVisibilioty = true);
    void Reset();
    DxCpuDescriptorHandle Push();
    
};