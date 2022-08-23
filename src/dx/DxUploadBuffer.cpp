#include "pch.h"
#include  "DxUploadBuffer.h"

#include "DxContext.h"

void DxPagePool::Initialize(uint32 sizeInBytes)
{
    pageSize = sizeInBytes;
    arena.Initialize(0, static_cast<uint64>(sizeof(DxPage)) * 512);
}

void DxPagePool::Reset()
{
    if(lastUsedPage) {
        lastUsedPage->next = freePages;
    }
    freePages = usedPages;
    usedPages = nullptr;
    lastUsedPage = nullptr;
}

void DxPagePool::ReturnPage(DxPage* page)
{
    mutex.lock();
    page->next = usedPages;
    usedPages = page;
    if(!lastUsedPage) {
        lastUsedPage = page;
    }
    mutex.unlock();
}

DxPage* DxPagePool::AllocateNewPage()
{
    mutex.lock();
    DxPage* result = arena.Allocate<DxPage>(1, true);
    
    mutex.unlock();
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(pageSize);
    CD3DX12_HEAP_PROPERTIES properties(D3D12_HEAP_TYPE_UPLOAD);
    CheckResult( dxContext.device->CreateCommittedResource(&properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&result->buffer)));
    result->gpuBasePtr = result->buffer->GetGPUVirtualAddress();
    //Map basically allows you to retrieve a CPU pointer of a GPU resource. It performs a few operations on the background so the data is up to date.
    //Depending on the type of resource you are mapping, you can perform different tasks like reading data or copying it.
    //Map basically return virtual pointer to GPU memory and when you access it OS/driver send data directly to resource
    result->buffer->Map(0,nullptr, (void**)&result->cpuBasePtr);
    result->pageSize = pageSize;
    return result;
    
}

DxPage* DxPagePool::GetFreePage()
{
    mutex.lock();
    DxPage* result = freePages;
    if(result) {
        freePages = freePages->next;
    }
    mutex.unlock();
    if(!result) {
        result = AllocateNewPage();
    }
    result->currentOffset = 0;
    return result;
}


DxAllocation DxUploadBuffer::Allocate(uint64 size, uint64 alignment)
{
    assert(size <= pagePool->pageSize);
    uint64 alignedOffset = currentPage ? AlignTo(currentPage->currentOffset, alignment) : 0;
    DxPage* page= currentPage;
    if(!page || alignedOffset + size > page->pageSize) {
        if(currentPage) pagePool->ReturnPage(currentPage);
        page = pagePool->GetFreePage();
        currentPage = page;
        alignedOffset = 0;
    }
    DxAllocation result;
    result.cpuPtr  = page->cpuBasePtr + alignedOffset;
    result.gpuPtr = page->gpuBasePtr + alignedOffset;
    result.resource = page->buffer;
    result.offsetInResource = (uint32)alignedOffset;
    page->currentOffset = alignedOffset + size;
    return result;
}


//called when context Initialize && newFrame 
void DxUploadBuffer::Reset()
{
    if(currentPage && pagePool) {
        pagePool->ReturnPage(currentPage);
    }
    currentPage = nullptr;
}

