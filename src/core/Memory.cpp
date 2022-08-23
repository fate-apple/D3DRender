#include "pch.h"
#include "Memory.h"

#include "math.h"


void MemoryArena::Initialize(uint64 inMinimumBlockSize, uint64 inReserveSize)
{
    Reset(true);

    this->minimumBlockSize = inMinimumBlockSize;
    this->reserveSize = inReserveSize;
    memory = (uint8*)VirtualAlloc(0, reserveSize, MEM_RESERVE, PAGE_READWRITE);
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    pageSize = systemInfo.dwPageSize;
    sizeLeftTotal = reserveSize;
}

void* MemoryArena::Allocate(uint64 size, uint64 alignment, bool clearToZero)
{
    if(size == 0) return nullptr;

    uint64 mask = alignment - 1;
    uint64 misAlignment = current & mask;
    uint64 adjustment = (misAlignment == 0) ? 0 : (alignment - misAlignment);

    current += adjustment;
    sizeLeftCurrent -= adjustment;
    sizeLeftTotal -= adjustment;
    assert(sizeLeftTotal >= size);
    EnsureFreeSize(size);
    uint8* result = memory + current;
    if(clearToZero) {
        memset(result, 0, size);
    }
    current += size;
    sizeLeftCurrent -= size;
    sizeLeftTotal -= size;
    return result;
}

void* MemoryArena::GetCurrent(uint64 alignment)
{
    return memory + AlignTo(current, alignment);
}

void MemoryArena::SetCurrentTo(void* ptr)
{
    current = (uint8*)ptr - memory;
    sizeLeftCurrent = committedMemory - current;
    sizeLeftTotal = reserveSize - current;
}


void MemoryArena::EnsureFreeSize(uint64 size)
{
    if(sizeLeftCurrent < size) {
        uint64 allocationSize = std::max(size, minimumBlockSize);
        allocationSize = pageSize * bucketize(allocationSize, pageSize);
        VirtualAlloc(memory + committedMemory, allocationSize, MEM_COMMIT, PAGE_READWRITE);
        sizeLeftCurrent += allocationSize;
        sizeLeftTotal += allocationSize;
        committedMemory += allocationSize;
    }
}



void MemoryArena::ResetToMarker(MemoryMarker marker)
{
    current = marker.before;
    sizeLeftCurrent = committedMemory - current;
    sizeLeftTotal = reserveSize - current;
}


void MemoryArena::Reset(bool freeMemory)
{
    if(memory && freeMemory) {
        VirtualFree(memory, 0, MEM_RELEASE);
        memory = nullptr;
        committedMemory = 0;
    }
    ResetToMarker({0});
}