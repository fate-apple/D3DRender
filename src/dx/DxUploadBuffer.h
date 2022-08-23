#pragma once
#include <mutex>

#include "Dx.h"
#include "core/Memory.h"

//store unit of Dx12Resource, which has pageSize and used to sync data;
struct DxPage
{
    Dx12Resource buffer;
    DxPage* next;

    uint8* cpuBasePtr;      //byte
    D3D12_GPU_VIRTUAL_ADDRESS gpuBasePtr;

    uint64 pageSize;
    uint64 currentOffset;
};

struct DxPagePool
{
    MemoryArena arena;
    uint64 pageSize;        //2MB in default;
    DxPage* freePages;
    DxPage* usedPages;
    DxPage* lastUsedPage;
    std::mutex mutex;
    
    void Initialize(uint32 sizeInBytes);
    void Reset();
    void ReturnPage(DxPage* page);
    DxPage* GetFreePage();
private:
    DxPage* AllocateNewPage();
};

// a range of upload data as continuous bytes. store in Dxpage;
struct DxAllocation
{
    void* cpuPtr;
    D3D12_GPU_VIRTUAL_ADDRESS gpuPtr;

    Dx12Resource resource;
    uint32 offsetInResource;
};

struct DxUploadBuffer
{
    DxPagePool* pagePool;       //belong to DxContext
    DxPage* currentPage;

    DxAllocation Allocate(uint64 size, uint64 aligment);
    void Reset();
};