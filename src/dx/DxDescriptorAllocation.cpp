#include "pch.h"
#include "DxDescriptorAllocation.h"

#include "DxContext.h"

void DxDescriptorHeap::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible, uint64 pageSize)
{
    this->type = type;
    this->_pageSize = pageSize;
    this->_shaderVisible = shaderVisible;
}

//try alloc in each page. if all false, create a new page
DxDescriptorAllocation DxDescriptorHeap::Allocate(uint64 count)
{
    if(count == 0) return{};

    assert(count<_pageSize);     

    for(uint32 i(0); i < _pages.size(); ++i) {
        auto [allocation, success] = _pages[i]->Allocate(count);
        if(success) {
            allocation._pageIndex = i;
            return allocation;
        }
    }
    
    _pages.push_back(new DxDescriptorPage(type, _pageSize, _shaderVisible));

    auto [allocation, success] = _pages.back()->Allocate(count);
    assert(success);
    allocation._pageIndex  = _pages.size()-1;
    
    return allocation;
}

void DxDescriptorHeap::Free(DxDescriptorAllocation allocation)
{
    if(allocation.IsValid()) {
        _pages[allocation._pageIndex]->Free(allocation);
    }
}

DxDescriptorPage::DxDescriptorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint64 capacity, bool shaderVisible)
{
    blockAllocator.Initialize(capacity);

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = (uint32)capacity;
    desc.Type = type;
    if(shaderVisible) {
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }

    CheckResult(dxContext.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    cpuBase = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    gpuBase = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    descriptorSize = dxContext.device->GetDescriptorHandleIncrementSize(type);
}

std::pair<DxDescriptorAllocation, bool> DxDescriptorPage::Allocate(uint64 count)
{
    uint64 offset =  blockAllocator.Allocate(count);
    if(offset == UINT64_MAX) {
        return {DxDescriptorAllocation{},false};
    }

    DxDescriptorAllocation result;
    result.count = count;
    //Offset -> += ; InitOffsetted -> =
    result._cpuBase.InitOffsetted(cpuBase, static_cast<INT>(offset), descriptorSize);
    result._gpuBase.InitOffsetted(gpuBase, static_cast<INT>(offset), descriptorSize);
    result._descriptorSize = descriptorSize;
    return {result, true};
}

void DxDescriptorPage::Free(DxDescriptorAllocation allocation)
{
    auto offset = allocation._cpuBase.ptr - cpuBase.ptr;
    assert(offset % descriptorSize == 0);
    offset /= descriptorSize;
    blockAllocator.Free(offset, allocation.count);
}

void DxFrameDescriptorAllocator::NewFrame(uint32 bufferedFrameID)
{
    mutex.lock();

    currentFrame = bufferedFrameID;

    while (usedPages[currentFrame])
    {
        DxFrameDescriptorPage* page = usedPages[currentFrame];
        usedPages[currentFrame] = page->next;
        page->next = freePages;
        freePages = page;
    }

    mutex.unlock();
}


void DxPushableDescriptorHeap::Initialize(uint32 maxSize, bool shaderVisibilioty)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = maxSize;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    if(shaderVisibilioty) {
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }
    CheckResult( dxContext.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));
    Reset();
}

void DxPushableDescriptorHeap::Reset()
{
    currentCpu = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    currentGpu = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

DxCpuDescriptorHandle DxPushableDescriptorHeap::Push()
{
    ++currentGpu;
    return currentCpu++;
}



